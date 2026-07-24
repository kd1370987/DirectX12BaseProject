#include "RenderGraph.h"

// マネージャー関連
#include "RGVarsionManager/RGResourceManager.h"
#include "../GraphicEngine.h"
#include "../RenderContext/RenderContext.h"
#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../RenderPassRegistry/RenderPassRegistry.h"

// オプション
#include "../../Option/OptionManager.h"

namespace Engine::Graphics
{
	RenderGraph::RenderGraph()
	{}
	RenderGraph::~RenderGraph()
	{}

	void RenderGraph::Init(RenderPassRegistry* a_pRenderPassRegister)
	{
		// リソースマネージャー作成
		m_upRGResourceManager = std::make_unique<RGResourceManager>();

		// パス登録
		for (auto& _pPassNode : a_pRenderPassRegister->RefPassNodes())
		{
			m_pPassNodeMap[_pPassNode->phase].push_back(_pPassNode.get());
		}

		// コンパイル
		Compile();
	}

	void RenderGraph::Release()
	{
		m_compiledPasses.clear();
		m_pPassNodeMap.clear();

		// 一時テクスチャ/バッファ(GBuffer・TAA・各種RTなど)のGPUリソースを解放する。
		// これらは個別のコミット済みリソースで数が多く(十数枚)、ここで解放しないと
		// 終了時のライブオブジェクト(LIVE_DEVICE)として残り続ける。
		if (m_upRGResourceManager)
		{
			m_upRGResourceManager->ReleasePhysicalResources();
		}
	}

	void RenderGraph::Compile()
	{
		m_compiledPasses.clear();
		m_sortedPasses.clear();

		// リソースの作成 (宣言)
		CreateResource();

		// 宣言された情報をもとに物理リソースを確保する
		m_upRGResourceManager->AllocateResources(D3D12::D3D12Wrapper::Instance().GetDevice());

		// 文字列からRGResourceHandleへと変換
		ResolveResourceHandles();

		// ソート配列の作成
		TopologicalSort();

		// コンパイルパスの作成 : バリア , バージョンの作成
		ComputeBarriersAndVersions();

		// 宣言をCPUディスクリプタまで焼き込む
		ResolveBindings();
	}

	bool RenderGraph::IsPassActive(const RenderPassNode* a_pNode) const
	{
		switch (a_pNode->frameParity)
		{
		case ERGFrameParity::Even:	return (m_tempralIndex % 2) == 0;
		case ERGFrameParity::Odd:	return (m_tempralIndex % 2) != 0;
		default:					return true;
		}
	}

	void RenderGraph::ApplyStaticBindings(RenderContext* a_pCtx, const CompiledPass& a_pass)
	{
		const auto* _pNode = a_pass.pNode;
		const bool _isCompute = (_pNode->pipelineType == ERGPipelineType::Compute);

		// ヒープ
		switch (_pNode->heapMode)
		{
		case ERGHeapMode::Default:				a_pCtx->BindHeap(); break;
		case ERGHeapMode::BindlessWithSampler:	a_pCtx->BindCopyHeapAndSumplerBindLess(); break;
		default: break;
		}

		// ルートシグネチャ
		if (_pNode->pRootSig)
		{
			if (_isCompute)	a_pCtx->SetComputeRootSignature(_pNode->pRootSig);
			else			a_pCtx->SetGraphicsRootSignature(_pNode->pRootSig);
		}

		// パイプラインステート
		if (_pNode->psoIndex != RenderPassNode::kInvalidPSOIndex)
		{
			if (_isCompute)	a_pCtx->SetComputePSO(_pNode->psoIndex);
			else			a_pCtx->SetGraphicPSO(_pNode->psoIndex);
		}

		// ルートシグネチャが無いとディスクリプタテーブルは張れない
		if (a_pass.binds.empty()) return;
		ENGINE_ERRLOG(
			_pNode->pRootSig != nullptr,
			"バインドを宣言していますがルートシグネチャが未設定です : %s",
			_pNode->name.c_str()
		);
		if (!_pNode->pRootSig) return;

		// 焼き込み済みディスクリプタをそのまま張る
		for (const auto& _bind : a_pass.binds)
		{
			std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> _handles(
				a_pass.descriptorTable.data() + _bind.firstHandle,
				_bind.count
			);

			switch (_bind.type)
			{
			case ERGBindType::SrvTable:
			case ERGBindType::Srv:
				if (_isCompute)	a_pCtx->ComputeBindSRV(_bind.rootIndex, _handles);
				else			a_pCtx->BindSRV(_bind.rootIndex, _handles);
				break;

			case ERGBindType::Uav:
				a_pCtx->BindUAV(_bind.rootIndex, _handles[0]);
				break;

			default: break;
			}
		}
	}

	void RenderGraph::Execute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		Swap();

		auto* _pCmdList = a_pCtx->GetCurrentCmdList();
		for (auto& _compilePass : m_compiledPasses)
		{
			Editor::MainEditor::Instance().StartWatch(_compilePass.pNode->name);

			// リソースバリア（UAVバリアも対応）
			// パスをスキップする場合でもリソースの状態遷移はコンパイル時の計算通りに進める
			for (auto& _barrier : _compilePass.preBarriers)
			{
				if (_barrier.isUAVBarrier)
				{
					a_pCtx->UAVBarrier(_barrier.pResource->GetResource());
				}
				else
				{
					_barrier.pResource->Barrier(_pCmdList, _barrier.after);
				}
			}

			// レンダーターゲット切り替え
			a_pCtx->SetRenderTargets(
				_compilePass.rtvHandles,
				_compilePass.hasDSV ? &_compilePass.dsvHandle : nullptr
			);

			// クリア
			for (auto& _index : _compilePass.clearRtvIndices)
			{
				a_pCtx->ClearRenderTarget(_compilePass.rtvHandles[_index]);
			}
			if (_compilePass.isDepthClear)
			{
				a_pCtx->ClearDSV(_compilePass.dsvHandle);
			}

			// 担当フレームでなければ中身は実行しない
			if (!IsPassActive(_compilePass.pNode))
			{
				Editor::MainEditor::Instance().EndWatch(_compilePass.pNode->name);
				continue;
			}

			// 宣言済みの静的なバインドを張る
			ApplyStaticBindings(a_pCtx, _compilePass);

			// 宣言済みのコピーを実行
			for (auto& [_pSrc, _pDst] : _compilePass.copies)
			{
				a_pCtx->ResourceCopy(_pSrc->GetResource(), _pDst->GetResource());
			}

			// パスの実行 : 静的に宣言しきれなかった部分だけ
			if (_compilePass.pNode->executeFunc)
			{
				_compilePass.pNode->executeFunc(a_pGE, a_pCtx, RGPassResources(&_compilePass));
			}

			Editor::MainEditor::Instance().EndWatch(_compilePass.pNode->name);
		}
	}

	const RGResourceManager* RenderGraph::GetRGResourceManager() const
	{
		return m_upRGResourceManager.get();
	}
	RGResourceManager* RenderGraph::RefRGResourceManager()
	{
		return m_upRGResourceManager.get();
	}

	// コンパイル時の内部処理
	void RenderGraph::TopologicalSort()
	{
		for (auto& [_phase, _pPassVec] : m_pPassNodeMap)
		{
			std::vector<RenderPassNode*> _sortedNodes = {};
			if (_phase == EDrawPhase::NotSort)
			{
				for (auto* _pPass : _pPassVec)
				{
					_sortedNodes.push_back(_pPass);
				}
			}
			else if (_phase == EDrawPhase::Particle)
			{
				// 外部のバッファなどはまだバリア未対応なので
				// ソートしない
				for (auto* _pPass : _pPassVec)
				{
					_sortedNodes.push_back(_pPass);
				}
			}
			else
			{
				// ソート配列の作成
				Algorithm::Graph::TopologicalSort(
					_pPassVec,
					_sortedNodes,
					[&](auto* a, auto* b)
					{
						// bの書き込みをaが読んでいれば依存がある
						for (const auto& _wAccess : b->accesses)
						{
							if (!_wAccess.isWrite) continue;

							for (const auto& _rAccess : a->accesses)
							{
								if (_rAccess.isWrite) continue;
								if (_wAccess.handle == _rAccess.handle) return true;
							}
						}
						return false;
					}
				);
			}

			// 合成
			m_sortedPasses.insert(m_sortedPasses.end(), _sortedNodes.begin(), _sortedNodes.end());
		}
	}

	void RenderGraph::ResolveResourceHandles()
	{
		// =========================================================
		// 宣言された文字列からリソースハンドルを解決する。
		// Read を先に全て処理してから Write を処理することで、
		// 同じパス内で読んでから書くリソースのバージョンが正しく進む
		for (auto& [_phase, _passNodeVec] : m_pPassNodeMap)
		{
			for (auto* _passNode : _passNodeVec)
			{
				for (auto& _access : _passNode->accesses)
				{
					if (_access.isWrite) continue;
					_access.handle = m_upRGResourceManager->Read(_access.resName);
				}

				for (auto& _access : _passNode->accesses)
				{
					if (!_access.isWrite) continue;
					_access.handle = m_upRGResourceManager->Write(_access.resName);
				}
			}
		}
	}

	void RenderGraph::ComputeBarriersAndVersions()
	{
		for (auto* _pass : m_sortedPasses)
		{
			CompiledPass _cp;
			_cp.pNode = _pass;

			std::vector<DXGI_FORMAT> _rtvFormats;
			DXGI_FORMAT _dsvFormat = DXGI_FORMAT_UNKNOWN;

			// Read を先に、Write を後に処理する。
			// 同じパスが読んでから書くリソースの遷移順を崩さないため
			std::vector<size_t> _order;
			_order.reserve(_pass->accesses.size());
			for (size_t _i = 0; _i < _pass->accesses.size(); ++_i)
			{
				if (!_pass->accesses[_i].isWrite) _order.push_back(_i);
			}
			for (size_t _i = 0; _i < _pass->accesses.size(); ++_i)
			{
				if (_pass->accesses[_i].isWrite) _order.push_back(_i);
			}

			// リソース遷移と RTV/DSV の解決
			for (size_t _orderIdx : _order)
			{
				auto& _access = _pass->accesses[_orderIdx];
				auto _handle = _access.handle;
				D3D12::GPUResource* _pPhysicalResource = m_upRGResourceManager->GetPhysicalResource(_handle);

				// クリアやDescriptorの取得
				if (_access.type == AccessType::RTV)
				{
					auto _rtvHandle = _pPhysicalResource->GetRTV();
					D3D12_CPU_DESCRIPTOR_HANDLE _rtv = D3D12::DescriptorHeapManager::Instance().GetCPU(_rtvHandle);
					_cp.rtvHandles.push_back(_rtv);

					_rtvFormats.push_back(m_upRGResourceManager->GetDXGIFormat(_handle));

					if (_access.load == LoadOp::Clear)
					{
						_cp.clearRtvIndices.push_back(_cp.rtvHandles.size() - 1);
					}
				}
				else if (_access.type == AccessType::Depth_Write || _access.type == AccessType::Depth_Read)
				{
					if (_access.type == AccessType::Depth_Read)
					{
						auto _dsvHandle = _pPhysicalResource->GetReadOnlyDSV();
						_cp.dsvHandle = D3D12::DescriptorHeapManager::Instance().GetCPU(_dsvHandle); // DSVOnry
					}
					else
					{
						auto _dsvHandle = _pPhysicalResource->GetDSV();
						_cp.dsvHandle = D3D12::DescriptorHeapManager::Instance().GetCPU(_dsvHandle); // DSV
					}

					_cp.hasDSV = true;

					_dsvFormat = m_upRGResourceManager->GetDXGIFormat(_handle);

					if (_access.load == LoadOp::Clear)
					{
						_cp.isDepthClear = true;
					}
				}

				// バリアの計算
				D3D12_RESOURCE_STATES _nextState = D3D12_RESOURCE_STATE_COMMON;
				switch (_access.type)
				{
				case AccessType::SRV:         _nextState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; break;
				case AccessType::RTV:         _nextState = D3D12_RESOURCE_STATE_RENDER_TARGET; break;
				case AccessType::UAV:         _nextState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; break;
				case AccessType::Depth_Read:  _nextState = D3D12_RESOURCE_STATE_DEPTH_READ; break;
				case AccessType::Depth_Write: _nextState = D3D12_RESOURCE_STATE_DEPTH_WRITE; break;
				case AccessType::CopySrc:     _nextState = D3D12_RESOURCE_STATE_COPY_SOURCE; break;
				case AccessType::CopyDst:     _nextState = D3D12_RESOURCE_STATE_COPY_DEST; break;
				default: break;
				}

				D3D12_RESOURCE_STATES _currentState = m_upRGResourceManager->GetCurrentState(_handle);

				bool _isStateChanged = (_currentState != _nextState);
				bool _isUAVtoUAV = (_currentState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && _nextState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				// ステートが変わったか、UAVの連続書き込みであればバリアを積む
				if (_isStateChanged || _isUAVtoUAV)
				{
					RGBarrier _barrier = {};
					_barrier.pResource = _pPhysicalResource;
					_barrier.before = _currentState;
					_barrier.after = _nextState;
					_barrier.isUAVBarrier = _isUAVtoUAV;

					_cp.preBarriers.push_back(_barrier);

					// リソースマネージャーの状態を更新
					m_upRGResourceManager->SetCurrentState(_handle, _nextState);
				}
			}

			// パスのインデックスを指定
			_cp.pNode->passIndex = static_cast<uint8_t>(m_compiledPasses.size());
			_cp.pNode->nameHash = StringUtility::ToHash(_cp.pNode->name);

			// パイプラインステート作成用クラス初期化
			_cp.pNode->pipelineBuilder.Init(_rtvFormats, _dsvFormat, _cp.pNode->nameHash);

			m_compiledPasses.push_back(_cp);
		}
	}

	void RenderGraph::ResolveBindings()
	{
		// =========================================================
		// ビルド時の宣言を、実行時にそのまま使える形へ焼き込む。
		// ここまで来ればリソース名の文字列は一切要らなくなる
		for (auto& _cp : m_compiledPasses)
		{
			auto* _pNode = _cp.pNode;
			const size_t _accessNum = _pNode->accesses.size();

			_cp.resources.resize(_accessNum, nullptr);
			_cp.descriptorTable.resize(_accessNum, { 0 });

			auto& _heapManager = D3D12::DescriptorHeapManager::Instance();

			for (size_t _i = 0; _i < _accessNum; ++_i)
			{
				const auto& _access = _pNode->accesses[_i];
				auto* _pRes = m_upRGResourceManager->GetPhysicalResource(_access.handle);
				_cp.resources[_i] = _pRes;

				if (!_pRes) continue;

				// アクセスタイプに対応したディスクリプタを引いておく
				switch (_access.type)
				{
				case AccessType::SRV:			_cp.descriptorTable[_i] = _heapManager.GetCPU(_pRes->GetSRV()); break;
				case AccessType::UAV:			_cp.descriptorTable[_i] = _heapManager.GetCPU(_pRes->GetUAV()); break;
				case AccessType::RTV:			_cp.descriptorTable[_i] = _heapManager.GetCPU(_pRes->GetRTV()); break;
				case AccessType::Depth_Write:	_cp.descriptorTable[_i] = _heapManager.GetCPU(_pRes->GetDSV()); break;
				case AccessType::Depth_Read:	_cp.descriptorTable[_i] = _heapManager.GetCPU(_pRes->GetSRV()); break;
				default: break;
				}
			}

			// バインド宣言はアクセス添字がそのままディスクリプタ添字になる
			_cp.binds.clear();
			for (const auto& _bind : _pNode->binds)
			{
				ENGINE_ERRLOG(
					static_cast<size_t>(_bind.firstAccess) + _bind.count <= _accessNum,
					"バインド宣言が宣言済みリソースの範囲外です : %s",
					_pNode->name.c_str()
				);

				_cp.binds.push_back({ _bind.type, _bind.rootIndex, _bind.firstAccess, _bind.count });
			}

			// コピー宣言も物理リソースまで解決しておく
			_cp.copies.clear();
			for (const auto& _copy : _pNode->copies)
			{
				_cp.copies.emplace_back(_cp.resources[_copy.srcAccess], _cp.resources[_copy.dstAccess]);
			}
		}
	}

	Resource::Texture* RenderGraph::GetTmepTexture(const std::string& a_texNmae)
	{
		for (auto& _tex : m_upRGResourceManager->GetTempTextures())
		{
			if (_tex->GetName() == a_texNmae)
			{
				return _tex.get();
			}
		}

		return nullptr;
	}

	RenderPassNode* RenderGraph::GetPass(UINT a_passHash)
	{
		for (auto* _pass : m_sortedPasses)
		{
			if (_pass->nameHash == a_passHash)
			{
				return _pass;
			}
		}

		ENGINE_ERRLOG(false,"パスが見つかりません : %d",a_passHash);
		return nullptr;
	}

	void RenderGraph::Swap()
	{
		m_tempralIndex = 1 - m_tempralIndex;
	}

	void RenderGraph::CreateResource()
	{
		auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		// =========================================================
		// 全パスからリソース要求を収集し、Usageをグローバルに合成する
		struct GlobalResourceInfo {
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			Resource::TextureUsage usage = Resource::TextureUsage::None;
			float texScale = 1.0f;
			bool isTemporal = false;
		};
		std::unordered_map<std::string, GlobalResourceInfo> _globalResourceMap = {};

		for (auto& [_phase, _passNodeVec] : m_pPassNodeMap)
		{
			for (auto* _passNode : _passNodeVec)
			{
				for (auto& _access : _passNode->accesses)
				{
					auto& _info = _globalResourceMap[_access.resName];
					if (_access.isTemporal) _info.isTemporal = true;

					if (!_access.isWrite)
					{
						// Read要求の収集
						if (_access.type == AccessType::Depth_Read) _info.usage |= Resource::TextureUsage::DSV;
						if (_access.type == AccessType::SRV)        _info.usage |= Resource::TextureUsage::SRV;
						if (_access.type == AccessType::UAV)        _info.usage |= Resource::TextureUsage::UAV;
					}
					else
					{
						// Write要求の収集（フォーマットとスケールはWrite側が主導権を持つ）
						if (_access.format != DXGI_FORMAT_UNKNOWN) _info.format = _access.format;
						_info.texScale = _access.texScale; // 念のためスケールも更新

						if (_access.type == AccessType::Depth_Write) _info.usage |= Resource::TextureUsage::DSV | Resource::TextureUsage::SRV;
						if (_access.type == AccessType::RTV)         _info.usage |= Resource::TextureUsage::RTV | Resource::TextureUsage::SRV;
						if (_access.type == AccessType::UAV)         _info.usage |= Resource::TextureUsage::UAV | Resource::TextureUsage::SRV;
					}
				}
			}
		}
		// =========================================================
		// 合成された情報をもとに、実際にリソースを Register する
		for (const auto& [_resName, _info] : _globalResourceMap)
		{
			// 外部から渡されるバッファ（スワップチェーン等）はフォーマット指定がないのでスキップ
			if (_info.format != DXGI_FORMAT_UNKNOWN)
			{
				m_upRGResourceManager->DeclareTexture(
					_resName,
					_info.format,
					static_cast<UINT64>(_winOp.windowWidth * _info.texScale),
					static_cast<UINT>(_winOp.windowHegiht * _info.texScale),
					_info.usage
				);
			}
		}
	}
}
