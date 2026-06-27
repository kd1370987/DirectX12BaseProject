#include "RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../MainEngine.h"

#include "../RenderGraph/RenderGraph.h"
#include "../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../GraphicEngine.h"
#include "ShapeDraw/ShapeDraw.h"

#include "../../Editor/SceneView/EditorCamera/EditorCamera.h"

#include "Engine/Scene/SceneManager/SceneManager.h"

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

		// ポインタのキャッシュ
		m_pShapeDraw = a_desc.pShapeRender;

		// ルート定数バッファアロケーター
		m_upCBAllocater = std::make_unique<CBAllocater>();
		m_upCBAllocater->RootCBVCreate(
			m_pDevice, a_desc.cbAllocatorMemSize
		);

		// バッファ作成
		m_instanceBuffer.Create(a_desc.pDevice, a_pCmdList, 1600, nullptr);						// インスタンスデータ
		m_subsetBuffer.Create(a_desc.pDevice, a_pCmdList, 1600, nullptr);						// サブセット情報用バッファ
		m_boneBuffer.Create(a_desc.pDevice, a_pCmdList, a_desc.boneElementNum, nullptr);		// ボーン行列用
		m_debugLineBuffer.Create(a_desc.pDevice, a_pCmdList, 10000, nullptr);					// 形状描画用バッファ

		// コピー戦略用SRVヒープの作成
		UINT _heapSize = D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeapSize();
		m_copyHeap.Create(
			m_pDevice,
			L"CopyHeap",
			_heapSize,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			0
		);
		m_bindLessHeap.Create(
			m_pDevice,
			L"BindLess",
			_heapSize,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			0
		);


		// クワッドポリゴン
		m_spQuadPolygon = std::make_shared<Resource::QuadPolygon>();
		m_spQuadPolygon->Init();
	}

	void RenderContext::Release()
	{
		// リンク解除
		m_pDevice = nullptr;		// デバイス
		m_pCmdList = nullptr;		// コマンドリスト

		// 形状描画用データ解放
		m_pShapeDraw = nullptr;
		m_spQuadPolygon.reset();

		// ルート定数バッファ用アロケーター解放
		m_upCBAllocater->Release();
		
		// ヒープ解放
		m_copyHeap.Release();
		m_bindLessHeap.Release();

		// 各構造体バッファ解放
		m_instanceBuffer.Release();
		m_subsetBuffer.Release();
		m_boneBuffer.Release();
		m_debugLineBuffer.Release();
	}

	void RenderContext::Clear()
	{
		m_pCmdList = nullptr;
		m_upCBAllocater->ResetUse();
		//m_pShapeDraw->Reset();

		// コピーヒープのリセット
		m_currentHeapOffset = 0;
	}

	ID3D12DescriptorHeap* RenderContext::GetCBV_SRV_UAVHeap() const
	{
		return m_copyHeap.GetHeap();
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


	CBAllocater* RenderContext::BindCB()
	{
		return m_upCBAllocater.get();
	}


	void RenderContext::ChangeRenderTarget(const std::vector<Handle<D3D12::RTV>>& a_rtvHandleVec, const Handle<D3D12::DSV>& a_dsvHandle)
	{
		// 変数用意
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _rtvCPUVec = {};
		D3D12_CPU_DESCRIPTOR_HANDLE _dsvCPU;
		D3D12_CPU_DESCRIPTOR_HANDLE* _pDSVCPU = nullptr;
		
		// RTVをハンドルへ変換
		for (auto& _rtv : a_rtvHandleVec)
		{
			_rtvCPUVec.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_rtv));
		}
		// 初期値じゃなければ
		if (a_dsvHandle != Handle<D3D12::DSV>())
		{
			_dsvCPU = D3D12::DescriptorHeapManager::Instance().GetCPU(a_dsvHandle);
			_pDSVCPU = &_dsvCPU;
		}

		// チェンジ
		m_pCmdList->OMSetRenderTargets(
			static_cast<UINT>(std::size(_rtvCPUVec)),
			_rtvCPUVec.data(),
			false,
			_pDSVCPU
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

	void RenderContext::BindSRV(UINT a_rootIdx, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHandles)
	{
		// コピー数取得
		UINT _count = static_cast<UINT>(a_cpuHandles.size());

		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset += _count;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return;

		// 確保した領域にコピーしていく
		for (UINT _i = 0; _i < _count; ++_i)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx + _i);

			if (a_cpuHandles[_i].ptr == 0) continue;

			// 一個ずつ連続した領域にコピー
			m_pDevice->CopyDescriptorsSimple(
				1,
				_destHandle,
				a_cpuHandles[_i],
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		// コマンドリストにバインド
		m_pCmdList->SetGraphicsRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::BindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle)
	{
		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset++;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return;

		D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx);

		// 一個ずつ連続した領域にコピー
		m_pDevice->CopyDescriptorsSimple(
			1,
			_destHandle,
			a_cpuHandle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		// コマンドリストにバインド
		m_pCmdList->SetGraphicsRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::BindSRV(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_srvHandle);
		BindSRV(a_rootIdx, _cpu);
	}

	void RenderContext::ComputeBindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle)
	{
		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset++;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return;

		D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx);

		// 一個ずつ連続した領域にコピー
		m_pDevice->CopyDescriptorsSimple(
			1,
			_destHandle,
			a_cpuHandle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		// コマンドリストにバインド
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::ComputeBindSRV(UINT a_rootIdx, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHandles)
	{
		// コピー数取得
		UINT _count = static_cast<UINT>(a_cpuHandles.size());

		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset += _count;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return;

		// 確保した領域にコピーしていく
		for (UINT _i = 0; _i < _count; ++_i)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx + _i);

			if (a_cpuHandles[_i].ptr == 0) continue;

			// 一個ずつ連続した領域にコピー
			m_pDevice->CopyDescriptorsSimple(
				1,
				_destHandle,
				a_cpuHandles[_i],
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		// コマンドリストにバインド
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::ComputeBindSRV(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_srvHandle);
		ComputeBindSRV(a_rootIdx, _cpu);
	}

	void RenderContext::BindUAV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle)
	{
		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset++;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return;

		D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx);

		// 一個ずつ連続した領域にコピー
		m_pDevice->CopyDescriptorsSimple(
			1,
			_destHandle,
			a_cpuHandle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		// コマンドリストにバインド
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::BindUAV(UINT a_rootIdx, Handle<D3D12::UAV> a_uavHandle)
	{
		auto _cpuHandle = D3D12::DescriptorHeapManager::Instance().GetCPU(a_uavHandle);
		BindUAV(a_rootIdx,_cpuHandle);
	}

	void RenderContext::BindUAV(UINT a_rootIdx, std::vector<Handle<D3D12::UAV>> a_uavHandles)
	{
		// コピー数取得
		UINT _count = static_cast<UINT>(a_uavHandles.size());

		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset += _count;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return;

		// 確保した領域にコピーしていく
		for (UINT _i = 0; _i < _count; ++_i)
		{
			auto _cpuHandle = D3D12::DescriptorHeapManager::Instance().GetCPU(a_uavHandles[_i]);
			D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx + _i);

			if (_cpuHandle.ptr == 0) continue;

			// 一個ずつ連続した領域にコピー
			m_pDevice->CopyDescriptorsSimple(
				1,
				_destHandle,
				_cpuHandle,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		// コマンドリストにバインド
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::BindUAVBindLess(UINT a_rootIdx, Handle<D3D12::UAV> a_handle)
	{
		// コマンドリストにバインド
		m_pCmdList->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_bindLessHeap.GetGPU(a_handle.GetIndex())
		);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle)
	{
		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset++;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return D3D12_GPU_DESCRIPTOR_HANDLE();

		D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx);

		// 一個ずつ連続した領域にコピー
		m_pDevice->CopyDescriptorsSimple(
			1,
			_destHandle,
			a_cpuHandle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		return m_copyHeap.GetGPU(_startIdx);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetGPUHandle(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles)
	{
		// コピー数取得
		UINT _count = static_cast<UINT>(a_cpuHandles.size());

		// 今の空きインデックスカウントを確保
		UINT _startIdx = m_currentHeapOffset;
		m_currentHeapOffset += _count;

		// ヒープサイズが足りなければリターン
		if (m_currentHeapOffset >= m_copyHeap.GetMaxSize())return D3D12_GPU_DESCRIPTOR_HANDLE();

		// 確保した領域にコピーしていく
		for (UINT _i = 0; _i < _count; ++_i)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE _destHandle = m_copyHeap.GetCPU(_startIdx + _i);

			// 一個ずつ連続した領域にコピー
			m_pDevice->CopyDescriptorsSimple(
				1,
				_destHandle,
				a_cpuHandles[_i],
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		// ハンドルを返す
		return m_copyHeap.GetGPU(_startIdx);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetGPUHandleBindLess(Handle<D3D12::SRV> a_handle)
	{
		return m_bindLessHeap.GetGPU(a_handle.GetIndex());
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


	void RenderContext::ClearDSV(const Handle<D3D12::DSV>& a_DSVHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_DSVHandle);
		D3D12::ClearDepthStencilView(m_pCmdList,_cpu);
	}


	ShapeRenderer* RenderContext::RefShapeDraw()
	{
		return m_pShapeDraw;
	}

	void RenderContext::BindHeap()
	{
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
			m_copyHeap.GetHeap()
		};
		m_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);
	}

	void RenderContext::BindCopyHeapAndSumpler()
	{
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
			m_copyHeap.GetHeap(),
			D3D12::DescriptorHeapManager::Instance().RefSamplerHeap()
		};
		m_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);
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
	}

	void RenderContext::BindHeaps(UINT a_numHeaps, ID3D12DescriptorHeap* const* a_pHeaps)
	{
		m_pCmdList->SetDescriptorHeaps(a_numHeaps,a_pHeaps);
	}

	void RenderContext::BindSRVBone()
	{
		BindSRV(6, m_boneBuffer.GetSRVHandle());
	}

	void RenderContext::Dispatch(UINT a_x, UINT a_y, UINT a_z)
	{
		m_pCmdList->Dispatch(a_x,a_y,a_z);
	}

	void RenderContext::BindGraphicsCamera()
	{
		if (!m_pGraphicsEngine) return;
		auto _cam = m_pGraphicsEngine->GetCameraData();
		GraphicsBindRootCBV<CameraData>(
			0,
			_cam
		);
	}



	void RenderContext::UpdateBuffer(
		const std::vector<InstanceData>& a_instanceVec, 
		const std::vector<SubSetData>& a_subsetVec
	)
	{
		// インスタンスデータバッファ
		if(!a_instanceVec.empty())
		{
			m_instanceBuffer.UpdateData(a_instanceVec.data(), a_instanceVec.size() * sizeof(InstanceData));
			m_instanceBuffer.Update(m_pCmdList);
		}

		// サブセットデータバッファ
		if(!a_subsetVec.empty())
		{
			m_subsetBuffer.UpdateData(a_subsetVec.data(), a_subsetVec.size() * sizeof(SubSetData));
			m_subsetBuffer.Update(m_pCmdList);
		}

		// ボーン行列の更新
		auto* _pCurrentWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (_pCurrentWorld->HasResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>())
		{
			auto& _boneMatPool = _pCurrentWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
			const auto& _data = _boneMatPool.GetAllData();
			m_boneBuffer.UpdateData(_data.data(), _data.size());
			m_boneBuffer.Update(m_pCmdList);
		}

		// デバッグライン用バッファ更新
		const auto& _debugVec = Editor::MainEditor::Instance().GetDebugLineDataVec();
		if (!_debugVec.empty())
		{
			m_debugLineBuffer.UpdateData(_debugVec.data(), _debugVec.size() * sizeof(DebugLineData));
			m_debugLineBuffer.Update(m_pCmdList);
		}
	}

	void RenderContext::BindIndex(UINT a_instanceBufferIndex, UINT a_subsetBufferIndex, UINT a_rootIndex)
	{
		BufferIndexData _indexData = {};
		_indexData.instanceIndex = a_instanceBufferIndex;
		_indexData.subsetIndex = a_subsetBufferIndex;
		// インデックスバインド
		BindCB()->BindAndAttachDataRootCBV<BufferIndexData>(
			m_pCmdList,
			a_rootIndex,
			_indexData
		);
	}

	void RenderContext::BindInstanceBuffer(UINT a_rootIndex)
	{
		BindSRV(a_rootIndex,m_instanceBuffer.GetSRVHandle());
	}

	void RenderContext::BindSubsetBuffer(UINT a_rootIndex)
	{
		BindSRV(a_rootIndex,m_subsetBuffer.GetSRVHandle());
	}

	void RenderContext::BindBonePalletBuffer(UINT a_rootIndex)
	{
		BindSRV(a_rootIndex,m_boneBuffer.GetSRVHandle());
	}

	void RenderContext::BindGraphicsDebugLineBuffer(UINT a_rootIndex)
	{
		BindSRV(a_rootIndex,m_debugLineBuffer.GetSRVHandle());
	}

	void RenderContext::TexCopy(
		const Handle<Resource::Texture>& a_src, const Handle<Resource::Texture>& a_dst
	)
	{
		auto* _srcTex = Resource::ResourceManager::Instance().Ref(a_src);
		auto* _dstTex = Resource::ResourceManager::Instance().Ref(a_dst);
		m_pCmdList->CopyResource(_dstTex->GetResource(), _srcTex->GetResource());
	}

	void RenderContext::SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_pCmdList->SetGraphicsRootSignature(a_pRootSig);
	}

	void RenderContext::SetComputeRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_pCmdList->SetComputeRootSignature(a_pRootSig);
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

	void RenderContext::SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri)
	{
		m_pCmdList->IASetPrimitiveTopology(a_pri);
	}

	void RenderContext::BindMaterialSRV(UINT a_index, const Resource::Material* a_pMaterial)
	{
		// SRVの送信
		std::vector<Handle<Resource::Texture>> _texVec = {};
		_texVec.push_back(a_pMaterial->baseColorTex);
		_texVec.push_back(a_pMaterial->metaRoughTex);
		_texVec.push_back(a_pMaterial->emissiveTex);
		_texVec.push_back(a_pMaterial->normalTex);
		BindSRV(a_index, _texVec);
	}

	void RenderContext::BindMaterialSRV(UINT a_index, uint16_t a_materialID)
	{
		const auto* _pMaterial = Resource::ResourceManager::Instance().Accece<Resource::Material>(a_materialID);
		if (!_pMaterial) return;

		std::vector<Handle<Resource::Texture>> _texVec = {};
		_texVec.push_back(_pMaterial->baseColorTex);
		_texVec.push_back(_pMaterial->metaRoughTex);
		_texVec.push_back(_pMaterial->emissiveTex);
		_texVec.push_back(_pMaterial->normalTex);
		BindSRV(a_index, _texVec);
		
	}

	void RenderContext::BindMesh(uint16_t a_meshID)
	{
		const auto* _pMesh = Resource::ResourceManager::Instance().Accece<Resource::Mesh>(a_meshID);
		if (!_pMesh) return;

		if (!_pMesh->HasRasterData()) return;
		const auto& _vertView = _pMesh->GetRasterData().vertexBuffer.GetView();
		const auto& _pIndexView = _pMesh->GetRasterData().indexBuffer.GetView();

		m_pCmdList->IASetVertexBuffers(0, 1, &_vertView);
		m_pCmdList->IASetIndexBuffer(&_pIndexView);
	}

	void RenderContext::Draw(const Resource::Mesh* a_pMesh, UINT a_subIdx)
	{
		// 描画
		UINT _faceCount = static_cast<UINT>(a_pMesh->GetMetaData().subsets[a_subIdx].faceCount);
		UINT _faceStart = static_cast<UINT>(a_pMesh->GetMetaData().subsets[a_subIdx].faceStart);
		m_pCmdList->DrawIndexedInstanced(
			_faceCount * 3, 1, _faceStart * 3, 0, 0
		);
	}

	void RenderContext::Draw(uint16_t a_meshID, UINT a_subIdx)
	{
		const auto* _pMesh = Resource::ResourceManager::Instance().Accece<Resource::Mesh>(a_meshID);
		if (!_pMesh) return;

		Draw(_pMesh, a_subIdx);
	}

	void RenderContext::DrawPolygonInstancing(UINT a_count)
	{
		// ポリゴンの頂点、インデックスバッファをバインド
		m_pCmdList->IASetVertexBuffers(0,1,&m_spQuadPolygon->GetVBView());
		m_pCmdList->IASetIndexBuffer(&m_spQuadPolygon->GetIBView());

		// GPUインスタンシング
		m_pCmdList->DrawIndexedInstanced(
			6,				// 頂点数
			a_count,		// 描画するオブジェクト数
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
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffarTex().GetRTV()
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

	void RenderContext::DrawQuad()
	{
		// コマンドリストの取得
		m_pCmdList->DrawInstanced(
			3, 1, 0, 0
		);
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