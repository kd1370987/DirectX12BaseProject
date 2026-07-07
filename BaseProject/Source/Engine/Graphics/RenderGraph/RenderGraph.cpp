#include "RenderGraph.h"

// マネージャー関連
#include "RGVarsionManager/RGResourceManager.h"
#include "../GraphicEngine.h"
#include "../RenderContext/RenderContext.h"
#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Resource/Loader/Texture/TextureLoader.h"
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
	}

	void RenderGraph::Compile()
	{
		m_compiledPasses.clear();
		m_sortedPasses.clear();

		// リソースの作成 (宣言)
		CreateResource();

		// 宣言された情報をもとに物理リソースを確保する
		m_upRGResourceManager->AllocateResources(D3D12::D3D12Wrapper::Instance().GetDevice());

		// 文字列からRGResourceHandeへと変換
		ResolveResourceHandles();

		// ソート配列の作成
		TopologicalSort();

		// コンパイルパスの作成 : バリア , バージョンの作成
		ComputeBarriersAndVersions();
	}

	void RenderGraph::Execute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		Swap();

		auto* _pCmdList = a_pCtx->GetCurrentCmdList();
		for (auto& _compilePass : m_compiledPasses)
		{
			Editor::MainEditor::Instance().StartWatch(_compilePass.pNode->name);

			// リソースバリア（UAVバリアも対応）
			for (auto& _barrier : _compilePass.preBarriers)
			{
				if (_barrier.isUAVBarrier)
				{
					// UAVバリアを発行 (※RenderContextにUAVBarrier関数を追加してください)
					a_pCtx->UAVBarrier(_barrier.pResource->GetResource());
				}
				else
				{
					// 通常のステート遷移バリア (※RenderContextのTransition関数がGPUResource*を受け取る想定)
					//a_pCtx->Transition(_barrier.pResource->GetResource(), _barrier.before, _barrier.after);
					_barrier.pResource->Barrier(_pCmdList,_barrier.after);
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

			// パスの実行
			if (_compilePass.pNode && _compilePass.pNode->executeFunc)
			{
				_compilePass.pNode->executeFunc(a_pGE, a_pCtx, _compilePass.pNode->passIndex);
			}

			Editor::MainEditor::Instance().EndWatch(_compilePass.pNode->name);
		}

		//m_upRGResourceManager->ResetForNextFrame(_pCmdList);
	}

	std::vector<DXGI_FORMAT> RenderGraph::GetPassRTVFormats(uint8_t a_passIndex)
	{
		std::vector<DXGI_FORMAT> _result = {};

		for (auto* _node : m_sortedPasses)
		{
			if (_node->passIndex == a_passIndex)
			{
				for (auto& _wRes : _node->writeRequests)
				{
					if (_wRes.type == AccessType::RTV)
					{
						_result.push_back(_wRes.format);
					}
				}
			}
		}
		return _result;
	}

	DXGI_FORMAT RenderGraph::GetPassDSVFormat(uint8_t a_passIndex)
	{
		DXGI_FORMAT _result = DXGI_FORMAT_UNKNOWN;

		for (auto* _node : m_sortedPasses)
		{
			if (_node->passIndex == a_passIndex)
			{
				for(auto& _wRes : _node->writeRequests)
				{
					if(_wRes.type == AccessType::Depth_Write)
					{
						_result = _wRes.format;
					}
					if (_result != DXGI_FORMAT_UNKNOWN) break;
				}
			}
			if (_result != DXGI_FORMAT_UNKNOWN) break;
		}
		return _result;
	}

	const RGResourceManager* RenderGraph::GetRGResourceManager() const
	{
		return m_upRGResourceManager.get();
	}
	RGResourceManager* RenderGraph::RefRGResourceManager()
	{
		return m_upRGResourceManager.get();
	}

	D3D12::GPUResource* RenderGraph::GetPassResource(uint8_t a_passIndex, const std::string& a_name) const
	{
		auto* _pNode = m_compiledPasses[a_passIndex].pNode;

		// Read要求から探す
		for (size_t i = 0; i < _pNode->readRequests.size(); ++i) {
			if (_pNode->readRequests[i].resName == a_name) {
				return m_upRGResourceManager->GetPhysicalResource(_pNode->read[i]);
			}
		}
		// Write要求から探す
		for (size_t i = 0; i < _pNode->writeRequests.size(); ++i) {
			if (_pNode->writeRequests[i].resName == a_name) {
				return m_upRGResourceManager->GetPhysicalResource(_pNode->write[i]);
			}
		}
		ENGINE_ERRLOG(false,"このパスで要求されていないリソース名です");
		return nullptr;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetPassSRV(uint8_t a_passIndex, const std::string& a_name) const
	{
		auto _pRes = GetPassResource(a_passIndex, a_name);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_pRes->GetSRV());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetPassUAV(uint8_t a_passIndex, const std::string& a_name) const
	{
		auto _pRes = GetPassResource(a_passIndex, a_name);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_pRes->GetUAV());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetPassDSV(uint8_t a_passIndex, const std::string& a_name) const
	{
		auto _pRes = GetPassResource(a_passIndex, a_name);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_pRes->GetDSV());
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
						// 依存があるかどうか
						for (auto& _write : b->write)
						{
							for (auto& _read : a->read)
							{
								if (_write == _read)
								{
									return true;
								}
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
		// 文字列から Resource::ID を取得し、配列に流し込む
		for (auto& [_phase, _passNodeVec] : m_pPassNodeMap)
		{
			for (auto* _passNode : _passNodeVec)
			{
				_passNode->read.clear();
				_passNode->write.clear();
				_passNode->resourceAccessVec.clear();

				// Read要求の解決
				for (auto& _req : _passNode->readRequests)
				{
					RGResourceHandle _handle = m_upRGResourceManager->Read(_req.resName);

					_passNode->read.push_back(_handle);
					_passNode->resourceAccessVec.push_back({ _handle, _req.type, _req.load, _req.store });
				}

				// Write要求の解決
				for (auto& _req : _passNode->writeRequests)
				{
					RGResourceHandle _handle = m_upRGResourceManager->Write(_req.resName);

					_passNode->write.push_back(_handle);
					_passNode->resourceAccessVec.push_back({ _handle, _req.type, _req.load, _req.store });
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

			// リソース遷移と RTV/DSV の解決
			for (auto& _access : _pass->resourceAccessVec)
			{
				auto _handle = _access.handle;
				D3D12::GPUResource* _pPhysicalResource = m_upRGResourceManager->GetPhysicalResource(_handle);

				// クリアやDescriptorの取得
				if (_access.type == AccessType::RTV)
				{
					// ※ 物理リソース(D3D12::GPUResource または Resource::Texture)からCPUハンドルを取得する関数がエンジン側にある想定
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
				// Read要求の収集
				for (auto& _req : _passNode->readRequests)
				{
					auto& _info = _globalResourceMap[_req.resName];
					if (_req.type == AccessType::Depth_Read) _info.usage |= Resource::TextureUsage::DSV;
					if (_req.type == AccessType::SRV)        _info.usage |= Resource::TextureUsage::SRV;
					if (_req.type == AccessType::UAV)        _info.usage |= Resource::TextureUsage::UAV;

					if (_req.isTemporal) _info.isTemporal = true;
				}

				// Write要求の収集（フォーマットとスケールはWrite側が主導権を持つ）
				for (auto& _req : _passNode->writeRequests)
				{
					auto& _info = _globalResourceMap[_req.resName];
					if (_req.isTemporal) _info.isTemporal = true;

					if (_req.format != DXGI_FORMAT_UNKNOWN) _info.format = _req.format;
					_info.texScale = _req.texScale; // 念のためスケールも更新
					
					if (_req.type == AccessType::Depth_Write) _info.usage |= Resource::TextureUsage::DSV | Resource::TextureUsage::SRV;
					if (_req.type == AccessType::RTV)         _info.usage |= Resource::TextureUsage::RTV | Resource::TextureUsage::SRV;
					if (_req.type == AccessType::UAV)         _info.usage |= Resource::TextureUsage::UAV | Resource::TextureUsage::SRV;

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