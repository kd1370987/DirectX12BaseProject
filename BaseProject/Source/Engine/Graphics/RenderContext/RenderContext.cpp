#include "RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../MainEngine.h"

#include "../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../GraphicEngine.h"
#include "../MeshBufferAllocator/MeshBufferAllocator.h"

#include "../../ECS/World/World.h"

//============================================================================================
//
// 初期化
//
//============================================================================================
namespace Engine::Graphics
{
	void RenderContext::Init(
		GraphicsEngine* a_pOwner,
		D3D12::GraphicsCommandList* a_pCmdList,
		const RenderContextDesc& a_desc
	)
	{
		m_pGraphicsEngine = a_pOwner;

		// デバイスのキャッシュ
		m_pDevice = a_desc.pDevice;

		// ルート定数バッファアロケーター
		m_upCBAllocator = std::make_unique<CBAllocator>();
		m_upCBAllocator->RootCBVCreate(
			m_pDevice, a_desc.cbAllocatorMemSize
		);

		// バッファ作成
		m_boneBuffer.Create(a_desc.pDevice, a_desc.boneElementNum);								// ボーン行列用
		m_debugLineBuffer.Create(a_desc.pDevice, a_pCmdList, 10000, nullptr);					// 形状描画用バッファ

		// メッシュ用データの作成
		m_meshInstanceBuffer.Create(a_desc.pDevice, a_pCmdList, 100000, nullptr);
		m_meshMaterialBuffer.Create(a_desc.pDevice, a_pCmdList, 100000, nullptr);

		// UIインスタンス
		m_uiInstanceBuffer.Create(a_desc.pDevice,10000);

		// コピー戦略用SRVヒープの作成
		UINT _heapSize = D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeapSize();
		m_copyHeap.Create(
			m_pDevice,
			L"CopyHeap",
			_heapSize,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			0
		);
		// ビンドレス用は先頭がグローバルヒープの丸写しで埋まる。
		// テーブルを張るぶんはその後ろへ確保しておく
		m_bindLessRingStart = _heapSize;
		m_bindLessHeap.Create(
			m_pDevice,
			L"BindLess",
			_heapSize + kBindLessRingSize,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			0
		);

		// 既定はコピー用ヒープをカレントにしておく
		m_pCurrentHeap = &m_copyHeap;

		// 描画用の板ポリはフレームごとに変わらないので、
		// コンテキストの数だけ作らずグラフィックスエンジンが1つずつ持っている
		// (GraphicsEngine::RefQuadPolygon / RefCurvedQuadPolygon)
	}

	void RenderContext::Release()
	{
		// リンク解除
		m_pDevice = nullptr;		// デバイス
		m_pCmdList = nullptr;		// コマンドリスト

		// ルート定数バッファ用アロケーター解放
		m_upCBAllocator->Release();
		
		// ヒープ解放
		m_copyHeap.Release();
		m_bindLessHeap.Release();

		// 各構造体バッファ解放
		m_boneBuffer.Release();
		m_debugLineBuffer.Release();
		m_meshInstanceBuffer.Release();
		m_meshMaterialBuffer.Release();
		m_uiInstanceBuffer.Release();
	}

	void RenderContext::Clear()
	{
		m_pCmdList = nullptr;
		m_upCBAllocator->ResetUse();
		//m_pShapeDraw->Reset();

		// リングのリセット。カレントヒープも既定へ戻す
		m_copyHeapOffset = 0;
		m_bindLessHeapOffset = m_bindLessRingStart;
		m_pCurrentHeap = &m_copyHeap;
	}

	D3D12::GraphicsCommandList* RenderContext::GetCurrentCmdList()
	{
		return m_pCmdList;
	}
	void RenderContext::SetDirectCommandList(D3D12::GraphicsCommandList* a_pCmdList)
	{
		m_pCmdList = a_pCmdList;
	}
	//============================================================================================
	//
	// カメラ
	//
	//============================================================================================


	CBAllocator* RenderContext::BindCB()
	{
		return m_upCBAllocator.get();
	}

	void RenderContext::SetRenderTargets(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_rtvHandleVec, const D3D12_CPU_DESCRIPTOR_HANDLE* a_pDsvHandle)
	{
		m_pCmdList->OMSetRenderTargets(
			static_cast<UINT>(a_rtvHandleVec.size()),
			a_rtvHandleVec.data(),
			false,
			a_pDsvHandle
		);

		// ビューポートとシザー矩形を設定
		m_pCmdList->RSSetViewports(1, &D3D12::D3D12Wrapper::Instance().GetViewport());
		m_pCmdList->RSSetScissorRects(1, &D3D12::D3D12Wrapper::Instance().GetScissorRect());
	}

	void RenderContext::BindSRV(
		UINT a_rootIdx,
		std::vector<Handle<Resource::Texture>>& a_texHandles
	)
	{
		// テクスチャからCPUハンドルを獲得する
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuHandles = {};
		for (auto& _texHandle : a_texHandles)
		{
			if (_texHandle == Handle<Resource::Texture>()) continue;
			const auto* _tex = Resource::ResourceManager::Instance().Get(_texHandle);
			if (!_tex) continue;
			_cpuHandles.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_tex->GetSRV()));
		}

		// バインド
		BindSRV(a_rootIdx,_cpuHandles);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::CopyToCurrentHeap(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		// 今の空きインデックスから連続領域を確保。
		// 数え上げはヒープごとに持つ(共用すると丸写しの上へ書いてしまう)
		const bool _isBindLess = (m_pCurrentHeap == &m_bindLessHeap);
		UINT& _offset = _isBindLess ? m_bindLessHeapOffset : m_copyHeapOffset;

		UINT _count = static_cast<UINT>(a_cpuHandles.size());
		UINT _startIdx = _offset;
		_offset += _count;

		// ヒープサイズが足りなければ無効ハンドルを返す
		if (_offset >= m_pCurrentHeap->GetMaxSize()) return D3D12_GPU_DESCRIPTOR_HANDLE{};

		// カレントヒープの確保領域へ1個ずつコピー(空ハンドルはスキップ)
		for (UINT _i = 0; _i < _count; ++_i)
		{
			if (a_cpuHandles[_i].ptr == 0) continue;

			m_pDevice->CopyDescriptorsSimple(
				1,
				m_pCurrentHeap->GetCPU(_startIdx + _i),
				a_cpuHandles[_i],
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		// 確保領域の先頭GPUハンドルを返す
		return m_pCurrentHeap->GetGPU(_startIdx);
	}

	void RenderContext::GraphicsBindTable(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE _gpu = CopyToCurrentHeap(a_cpuHandles);
		if (_gpu.ptr == 0) return;	// 容量オーバー時はバインドしない
		m_pCmdList->SetGraphicsRootDescriptorTable(a_rootIdx, _gpu);
	}

	void RenderContext::ComputeBindTable(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE _gpu = CopyToCurrentHeap(a_cpuHandles);
		if (_gpu.ptr == 0) return;	// 容量オーバー時はバインドしない
		m_pCmdList->SetComputeRootDescriptorTable(a_rootIdx, _gpu);
	}

	void RenderContext::BindSRV(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		GraphicsBindTable(a_rootIdx, a_cpuHandles);
	}

	void RenderContext::BindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle)
	{
		GraphicsBindTable(a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE>(&a_cpuHandle, 1));
	}

	void RenderContext::BindSRV(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_srvHandle);
		BindSRV(a_rootIdx, _cpu);
	}

	void RenderContext::ComputeBindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle)
	{
		ComputeBindTable(a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE>(&a_cpuHandle, 1));
	}

	void RenderContext::ComputeBindSRV(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		ComputeBindTable(a_rootIdx, a_cpuHandles);
	}

	void RenderContext::ComputeBindSRV(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_srvHandle);
		ComputeBindSRV(a_rootIdx, _cpu);
	}

	void RenderContext::ComputeBindSRVBindLess(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle)
	{
		// バインドレスヒープはカレントヒープ(=m_bindLessHeap)を直接インデックスで引く
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_pCurrentHeap->GetGPU(a_srvHandle.GetIndex())
		);
	}

	void RenderContext::BindUAV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle)
	{
		ComputeBindTable(a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE>(&a_cpuHandle, 1));
	}

	void RenderContext::BindUAV(UINT a_rootIdx, Handle<D3D12::UAV> a_uavHandle)
	{
		auto _cpuHandle = D3D12::DescriptorHeapManager::Instance().GetCPU(a_uavHandle);
		BindUAV(a_rootIdx,_cpuHandle);
	}

	void RenderContext::BindUAV(UINT a_rootIdx, std::vector<Handle<D3D12::UAV>> a_uavHandles)
	{
		// ハンドル配列をCPUハンドル配列へ変換してまとめてバインド
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuHandles = {};
		_cpuHandles.reserve(a_uavHandles.size());
		for (const auto& _handle : a_uavHandles)
		{
			_cpuHandles.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_handle));
		}
		ComputeBindTable(a_rootIdx, _cpuHandles);
	}

	void RenderContext::BindUAVBindLess(UINT a_rootIdx, Handle<D3D12::UAV> a_handle)
	{
		// バインドレスヒープはカレントヒープ(=m_bindLessHeap)を直接インデックスで引く
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_pCurrentHeap->GetGPU(a_handle.GetIndex())
		);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetGPUHandle(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		return CopyToCurrentHeap(a_cpuHandles);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetGPUHandleBindLess(Handle<D3D12::SRV> a_handle)
	{
		return m_pCurrentHeap->GetGPU(a_handle.GetIndex());
	}

	void RenderContext::ClearRenderTarget(const Handle<Resource::Texture>& a_texHandle)
	{
		auto* _tex = Resource::ResourceManager::Instance().Ref(a_texHandle);

		// もしテクスチャのステートがレンダーターゲットでなければリターン
		if (
			_tex->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET && 
			!Resource::HasFlag(_tex->GetUsage(),Resource::TextureUsage::RTV)
		)
		{
			return;
		}
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(_tex->GetRTV());

		// CPUハンドルと、テクスチャ作成時のクリアバリューをセット
		D3D12::ClearRenderTargetView(m_pCmdList, _cpu, _tex->GetClearColor());
	}

	void RenderContext::ClearRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE& a_rtvHandle, const DirectX::XMFLOAT4& a_clearColor)
	{
		D3D12::ClearRenderTargetView(m_pCmdList, a_rtvHandle, a_clearColor);
	}


	void RenderContext::ClearDSV(const Handle<D3D12::DSV>& a_DSVHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_DSVHandle);
		D3D12::ClearDepthStencilView(m_pCmdList,_cpu);
	}

	void RenderContext::ClearDSV(const D3D12_CPU_DESCRIPTOR_HANDLE& a_DSVHandle)
	{
		D3D12::ClearDepthStencilView(m_pCmdList,a_DSVHandle);
	}

	void RenderContext::BindHeap()
	{
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
			m_copyHeap.GetHeap()
		};
		m_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

		// セットしたヒープをカレントとしてキャッシュ
		m_pCurrentHeap = &m_copyHeap;
	}

	void RenderContext::BindCopyHeapAndSumplerBindLess()
	{
		ID3D12DescriptorHeap* _srcHeap = D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeap();

		// ヒープ丸ごとコピー
		UINT _heapNum = D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeapSize();
		m_pDevice->CopyDescriptorsSimple(
			_heapNum,
			m_bindLessHeap.GetCPU(0),
			_srcHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
			m_bindLessHeap.GetHeap(),
			D3D12::DescriptorHeapManager::Instance().RefSamplerHeap()
		};
		m_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

		// セットしたヒープをカレントとしてキャッシュ
		m_pCurrentHeap = &m_bindLessHeap;
	}

	void RenderContext::Dispatch(UINT a_x, UINT a_y, UINT a_z)
	{
		m_pCmdList->Dispatch(a_x,a_y,a_z);
	}

	void RenderContext::DispatchMesh(UINT a_x, UINT a_y, UINT a_z)
	{
		m_pCmdList->DispatchMesh(a_x,a_y,a_z);
	}

	void RenderContext::UpdateBuffer(
		const std::vector<MeshInstanceData>& a_mesInstance,
		const std::vector<MeshMaterial>& a_mesMaterial,
		const std::vector<Resource::BoneMatrix>& a_boneMatVec)
	{
		// インスタンスデータバッファ
		if (!a_mesInstance.empty())
		{
			m_meshInstanceBuffer.UpdateData(a_mesInstance.data(),a_mesInstance.size() * sizeof(MeshInstanceData));
			m_meshInstanceBuffer.Update(m_pCmdList);
		}
		// マテリアルデータバッファ
		if (!a_mesMaterial.empty())
		{
			m_meshMaterialBuffer.UpdateData(a_mesMaterial.data(),a_mesMaterial.size() * sizeof(MeshMaterial));
			m_meshMaterialBuffer.Update(m_pCmdList);
		}

		// ボーン行列の更新
		//
		// 中身は呼び出し側(GraphicsEngine)が用意する。
		// 以前はここで SceneManager::RefWorld() =「一番上のシーン」から直接引いていたが、
		// ポーズ画面のようにシーンを重ねているとポーズ側のワールドしか載らず、
		// 後ろのゲームのキャラがボーン行列を失って一点に潰れ、消えたように見えていた。
		m_boneBuffer.ResetForNewFrame();
		if (!a_boneMatVec.empty())
		{
			m_boneBuffer.AllocateAndWrite(a_boneMatVec.data(), static_cast<UINT>(a_boneMatVec.size()));
		}

		// デバッグライン用バッファ更新
		const auto& _debugVec = Editor::MainEditor::Instance().GetDebugLineDataVec();
		if (!_debugVec.empty())
		{
			m_debugLineBuffer.UpdateData(_debugVec.data(), _debugVec.size() * sizeof(DebugLineData));
			m_debugLineBuffer.Update(m_pCmdList);
		}
	}

	void RenderContext::UpdateUIBuffer(const std::vector<UIData>& a_uiInstanceVec)
	{
		if (a_uiInstanceVec.empty()) return;

		// 書き込みオフセットを毎フレーム先頭へ戻す。
		// BindUIBuffer() はバッファ先頭(element0)のGPUアドレスを固定でバインドするため、
		// リセットしないと AllocateAndWrite が毎フレーム後方へ書き進み、
		// シェーダーは初回フレームのデータ(element0)を読み続けてUIが動かなくなる。
		// (ボーン用 m_boneBuffer と同じ運用に揃える)
		m_uiInstanceBuffer.ResetForNewFrame();
		m_uiInstanceBuffer.AllocateAndWrite(a_uiInstanceVec);
	}

	void RenderContext::ComputeBindBonePalletBuffer(UINT a_rootIndex)
	{
		ComputeBindSRV(a_rootIndex, m_boneBuffer.GetSRV());
	}

	void RenderContext::BindGraphicsDebugLineBuffer(UINT a_rootIndex)
	{
		BindSRV(a_rootIndex,m_debugLineBuffer.GetSRVHandle());
	}

	void RenderContext::BindCamera()
	{
		if (!m_pGraphicsEngine) return;
		const auto& _cam = m_pGraphicsEngine->GetCameraData();
		GraphicsBindRootCBV(0, _cam);
	}

	void RenderContext::BindMeshInstance()
	{
		m_pCmdList->SetGraphicsRootShaderResourceView(1, m_meshInstanceBuffer.GetGPUVirtualAddress());
		m_pCmdList->SetGraphicsRootShaderResourceView(2,m_meshMaterialBuffer.GetGPUVirtualAddress());
	}

	void RenderContext::BindMeshlet()
	{
		auto* _pBufferManager = m_pGraphicsEngine->RefMeshBufferAllocator();
		if (!_pBufferManager)return;
		m_pCmdList->SetGraphicsRootShaderResourceView(3, _pBufferManager->RefMeshletBuffer().GetResource()->GetGPUVirtualAddress());
		m_pCmdList->SetGraphicsRootShaderResourceView(4, _pBufferManager->RefUniqueVertexIndicesBuffer().GetResource()->GetGPUVirtualAddress());
		m_pCmdList->SetGraphicsRootShaderResourceView(5, _pBufferManager->RefMeshletTriangleBuffer().GetResource()->GetGPUVirtualAddress());
		m_pCmdList->SetGraphicsRootShaderResourceView(6, _pBufferManager->GetStaticVertexBuffer().GetResource()->GetGPUVirtualAddress());
		m_pCmdList->SetGraphicsRootShaderResourceView(7, _pBufferManager->GetAnimatedVertexBuffer().GetResource()->GetGPUVirtualAddress());
		m_pCmdList->SetGraphicsRootShaderResourceView(8, _pBufferManager->RefMeshletCullDataBuffer().GetResource()->GetGPUVirtualAddress());
		// 前フレームのスキニング済み頂点(t8 = ルートパラメータ10) : モーションベクター用
		m_pCmdList->SetGraphicsRootShaderResourceView(10, _pBufferManager->GetPrevAnimatedVertexBuffer().GetResource()->GetGPUVirtualAddress());
	}

	void RenderContext::BindUIBuffer(UINT a_rootIndex, UINT a_startInstance)
	{
		// ルートSRVは「先頭要素のアドレス」を渡すだけなので、
		// 途中から張りたいときは要素ぶんバイト数を進める
		const D3D12_GPU_VIRTUAL_ADDRESS _address =
			m_uiInstanceBuffer.GetGPUVirtualAddress() +
			static_cast<UINT64>(a_startInstance) * sizeof(UIData);

		m_pCmdList->SetGraphicsRootShaderResourceView(a_rootIndex, _address);
	}

	// UAVのテクスチャを塗りつぶす
	void RenderContext::ClearUAV(
		D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle,
		ID3D12Resource* a_pResource,
		const float a_color[4])
	{
		if (!m_pCmdList || !a_pResource) return;
		if (a_cpuHandle.ptr == 0) return;

		// GPUハンドルはシェーダー可視ヒープの上にしか作れない
		BindHeap();

		const D3D12_GPU_DESCRIPTOR_HANDLE _gpu =
			CopyToCurrentHeap(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE>(&a_cpuHandle, 1));
		if (_gpu.ptr == 0) return;

		m_pCmdList->ClearUnorderedAccessViewFloat(_gpu, a_cpuHandle, a_pResource, a_color, 0, nullptr);
	}

	void RenderContext::DrawUI(UINT a_rootIndex)
	{
		const auto& _uiDataVec = m_pGraphicsEngine->GetUIDataBuffer();
		if (_uiDataVec.empty()) return;

		auto* _pFlatPolygon   = m_pGraphicsEngine->RefQuadPolygon();
		auto* _pCurvedPolygon = m_pGraphicsEngine->RefCurvedQuadPolygon();
		if (!_pFlatPolygon || !_pCurvedPolygon) return;

		//------------------------------------------------------------------
		// 湾曲するUIだけ、横に分割した板ポリで描く
		//
		// 使う板ポリが変わる = 別のドローになるので、湾曲の有無で2回に分ける。
		// ただし配列はレイヤー順に並んでいて、UIパスは深度を持たない(積んだ順が
		// そのまま重なり)ので、「湾曲するものを全部まとめて後から描く」ことはできない。
		// 前後関係が入れ替わって、下にあるはずのUIが手前に出てしまう。
		//
		// そこで同じ種類が続く区間ごとに1回ずつ描く。並び順はそのままなので重なりは崩れず、
		// 湾曲UIが混ざっていないフレームは今までどおり1回のドローで終わる
		//------------------------------------------------------------------
		size_t _runStart = 0;
		while (_runStart < _uiDataVec.size())
		{
			const bool _isCurved = _uiDataVec[_runStart].IsCurved();

			// 同じ種類が続くところまで伸ばす
			size_t _runEnd = _runStart + 1;
			while (_runEnd < _uiDataVec.size() && _uiDataVec[_runEnd].IsCurved() == _isCurved)
			{
				++_runEnd;
			}

			// SV_InstanceID は毎回0から数え直されるので、区間の頭を先頭にして張り直す
			BindUIBuffer(a_rootIndex, static_cast<UINT>(_runStart));

			DrawPolygonInstancing(
				_isCurved ? _pCurvedPolygon : _pFlatPolygon,
				static_cast<UINT>(_runEnd - _runStart)
			);

			_runStart = _runEnd;
		}
	}

	void RenderContext::DrawQueueDispathMesh(uint8_t a_passIndex)
	{
		// キャッシュ
		uint8_t _lastPSO = 0xFF;

		// 指定タイプの命令キューを取得
		auto _itemVec = m_pGraphicsEngine->GetPassItems(a_passIndex);
		if (_itemVec.empty()) return;

		for (auto& _item : _itemVec)
		{
			// メッシュシェーダー経路はインスタンスデータ側にリソースを寄せてあるため、
			// ここではメッシュ・マテリアルをバインドしない
			uint8_t  _psoID = _item.GetPSOID();
			// ----------------------------------------------------
			// PSOの切り替え
			// ----------------------------------------------------
			if (_psoID != _lastPSO)
			{
				auto* _pPSO = MainEngine::Instance().RefPipelineManager()->GetPSO(_psoID);
				if (!_pPSO) continue;
				SetGraphicPSO(_pPSO);

				_lastPSO = _psoID;
			}
			m_pCmdList->SetGraphicsRoot32BitConstant(9,_item.meshInstanceIndex,0);
			m_pCmdList->DispatchMesh((_item.subsetMeshletCount + 31) / 32, 1, 1);
		}
	}

	void RenderContext::ResourceCopy(ID3D12Resource* a_pSrc, ID3D12Resource* a_pDst)
	{
		m_pCmdList->CopyResource(a_pDst, a_pSrc);
	}

	void RenderContext::SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_pCmdList->SetGraphicsRootSignature(a_pRootSig);
	}

	void RenderContext::SetComputeRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_pCmdList->SetComputeRootSignature(a_pRootSig);
	}

	void RenderContext::SetGraphicsRootSignature(const Handle<ID3D12RootSignature>& a_handle)
	{
		auto* _pPsoManager = MainEngine::Instance().RefPipelineManager();
		if (!_pPsoManager) return;
		auto* _pRootSig = _pPsoManager->GetRootSignature(a_handle);
		if (!_pRootSig) return;
		SetGraphicsRootSignature(_pRootSig);
	}

	void RenderContext::SetComputeRootSignature(const Handle<ID3D12RootSignature>& a_handle)
	{
		auto* _pPsoManager = MainEngine::Instance().RefPipelineManager();
		if (!_pPsoManager) return;
		auto* _pRootSig = _pPsoManager->GetRootSignature(a_handle);
		if (!_pRootSig) return;
		SetComputeRootSignature(_pRootSig);
	}



	void RenderContext::SetGraphicPSO(ID3D12PipelineState* a_pPSO)
	{
		m_pCmdList->SetPipelineState(a_pPSO);
		// プリミティブトポロジーセット
		m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void RenderContext::SetGraphicPSO(uint8_t a_pPsoIndex)
	{
		auto* _pPsoManager = MainEngine::Instance().RefPipelineManager();
		if (!_pPsoManager) return;
		auto* _pPSO = _pPsoManager->GetPSO(a_pPsoIndex);
		if (!_pPSO) return;
		SetGraphicPSO(_pPSO);
	}

	void RenderContext::SetComputePSO(ID3D12PipelineState* a_pPSO)
	{
		m_pCmdList->SetPipelineState(a_pPSO);
	}

	// ハンドルで張る版。
	// 8bitの添字と違って世代まで見るので、無効なものは黙って弾ける
	void RenderContext::SetGraphicPSO(const Handle<ID3D12PipelineState>& a_handle)
	{
		auto* _pPsoManager = MainEngine::Instance().RefPipelineManager();
		if (!_pPsoManager) return;
		auto* _pPSO = _pPsoManager->GetPSO(a_handle);
		if (!_pPSO) return;
		SetGraphicPSO(_pPSO);
	}

	void RenderContext::SetComputePSO(const Handle<ID3D12PipelineState>& a_handle)
	{
		auto* _pPsoManager = MainEngine::Instance().RefPipelineManager();
		if (!_pPsoManager) return;
		auto* _pPSO = _pPsoManager->GetPSO(a_handle);
		if (!_pPSO) return;
		SetComputePSO(_pPSO);
	}

	void RenderContext::SetComputePSO(uint8_t a_pPsoIndex)
	{
		auto* _pPsoManager = MainEngine::Instance().RefPipelineManager();
		if (!_pPsoManager) return;
		auto* _pPSO = _pPsoManager->GetPSO(a_pPsoIndex);
		if (!_pPSO) return;
		SetComputePSO(_pPSO);
	}

	void RenderContext::SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri)
	{
		m_pCmdList->IASetPrimitiveTopology(a_pri);
	}

	void RenderContext::DrawPolygonInstancing(UINT a_count)
	{
		DrawPolygonInstancing(m_pGraphicsEngine->RefQuadPolygon(), a_count);
	}

	void RenderContext::DrawPolygonInstancing(Resource::QuadPolygon* a_pPolygon, UINT a_count)
	{
		if (!a_pPolygon) return;

		// ポリゴンの頂点、インデックスバッファをバインド
		const D3D12_VERTEX_BUFFER_VIEW& _vbView = a_pPolygon->GetVBView();
		const D3D12_INDEX_BUFFER_VIEW& _ibView = a_pPolygon->GetIBView();
		m_pCmdList->IASetVertexBuffers(0,1,&_vbView);
		m_pCmdList->IASetIndexBuffer(&_ibView);

		const UINT _indexByteSize = (_ibView.Format == DXGI_FORMAT_R16_UINT) ? 2u : 4u;
		const UINT _indexCount = _ibView.SizeInBytes / _indexByteSize;

		// GPUインスタンシング
		m_pCmdList->DrawIndexedInstanced(
			_indexCount,	// インデックス数(4頂点の1枚板なら6、分割板ならその分だけ増える)
			a_count,		// 描画するオブジェクト数(インスタンス数)
			0,
			0,
			0
		);
	}

	void RenderContext::Transition(
		ID3D12Resource* a_pResource,
		D3D12_RESOURCE_STATES a_before,
		D3D12_RESOURCE_STATES a_after
	)
	{
		D3D12::ResourceBarrier(
			m_pCmdList,
			a_pResource,
			a_before,
			a_after
		);
	}


	void RenderContext::ChangeBackBuffer()
	{
		// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
		auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBufferTex().GetRTV()
		);

		// レンダーターゲットを設定
		m_pCmdList->OMSetRenderTargets(
			1,
			&_cpuHandle,
			FALSE,
			nullptr
		);

		// ビューポートとシザー矩形を設定
		m_pCmdList->RSSetViewports(1, &D3D12::D3D12Wrapper::Instance().GetViewport());
		m_pCmdList->RSSetScissorRects(1, &D3D12::D3D12Wrapper::Instance().GetScissorRect());

		// バッファクリア
		const float _clearColor[] = { 0.0f,0.0f,0.0f,1.0f };
		m_pCmdList->ClearRenderTargetView(_cpuHandle, _clearColor, 0, nullptr);		// レンダーターゲット
	}

	void RenderContext::DrawShape()
	{
		const auto& _debugVec = Editor::MainEditor::Instance().GetDebugLineDataVec();
		if (_debugVec.empty()) return;
		m_pCmdList->DrawInstanced(
			136,
			static_cast<UINT>(_debugVec.size()),
			0,
			0
		);
	}

	RenderContext::RenderContext()
	{}

	RenderContext::~RenderContext()
	{}
}