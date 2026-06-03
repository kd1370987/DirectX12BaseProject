#include "RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/D3D12//D3DObject/CommandList/CommandList.h"


#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Animation/AnimationMatrixManager/AnimationMatrixManager.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "../RenderGraph/RenderGraph.h"

#include "ShapeDraw/ShapeDraw.h"

#include "../../Editor/SceneView/EditorCamera/EditorCamera.h"
//============================================================================================
//
// 初期化
//
//============================================================================================
namespace Engine::Graphics
{
	void RenderContext::Init(const RenderContextDesc& a_desc)
	{
		D3D12::D3D12Wrapper::Instance().CommandQueueReset();
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetCmdList();

		// デバイスのキャッシュ
		m_pDevice = a_desc.pDevice;

		// ポインタのキャッシュ
		m_pShapeDraw = a_desc.pShapeRender;

		// 形状描画用バッファ作成
		m_shapeVertexBuffer.Create(m_pDevice,m_pShapeDraw->GetMaxCount());

		// ルート定数バッファアロケーター
		m_upCBAllocater = std::make_unique<CBAllocater>();
		m_upCBAllocater->RootCBVCreate(
			m_pDevice, a_desc.cbAllocatorMemSize
		);

		// バッファ作成
		m_instanceBuffer.Create(a_desc.pDevice, *_pCmdList, 1600, nullptr);
		m_subsetBuffer.Create(a_desc.pDevice, *_pCmdList, 1600, nullptr);
		m_boneBuffer.Create(a_desc.pDevice, *_pCmdList, a_desc.boneElementNum, nullptr);


		// 構造体バッファ作成のためGPU操作を実行
		D3D12::D3D12Wrapper::Instance().CloseAndExecuteComdLists(_pCmdList);

		UINT _heapSize = D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeapSize();

		// コピー戦略用SRVヒープの作成
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
	

	void RenderContext::Begine(const FrameDesc& a_desc)
	{
		Clear();

		//m_pCmdList = a_desc.pCmdList;
		m_pCmdList = a_desc.pCmdListClass;
		
	}

	void RenderContext::Clear()
	{
		m_pCmdList = nullptr;
		m_upCBAllocater->ResetUse();
		m_pShapeDraw->Reset();

		// コピーヒープのリセット
		m_currentHeapOffset = 0;
	}

	ID3D12DescriptorHeap* RenderContext::GetCBV_SRV_UAVHeap() const
	{
		return m_copyHeap.GetHeap();
	}

	D3D12::CommandList* RenderContext::GetCurrentCmdList()
	{
		return m_pCmdList;
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

	void RenderContext::BindVoidRootCBV(UINT a_index, const void* a_pData, size_t a_size)
	{
		m_upCBAllocater->BindAndAttachDataRootCBV(m_pCmdList->NGet(), a_index, a_pData, a_size);
	}

	void RenderContext::ChangeRenderTarget(const std::vector<Resource::Handle<D3D12::RTV>>& a_rtvHandleVec, const Resource::Handle<D3D12::DSV>& a_dsvHandle)
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
		if (a_dsvHandle != Resource::Handle<D3D12::DSV>())
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

		// ビューポートとシザー矩形をセット
		D3D12::D3D12Wrapper::Instance().SetViewportAndRect();
	}

	void RenderContext::BindSRV(
		UINT a_rootIdx,
		std::vector<Resource::Handle<Resource::Texture>>& a_texHandles
	)
	{
		// テクスチャからCPUハンドルを獲得する
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuHandles = {};
		for (auto& _texHandle : a_texHandles)
		{
			if (_texHandle == Resource::Handle<Resource::Texture>()) continue;
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
		//m_pCmdList->NGet()->SetGraphicsRootDescriptorTable(
		//	a_rootIdx,
		//	m_copyHeap.GetGPU(_startIdx)
		//);
		m_pCmdList->SetGraphicsRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::BindSRV(UINT a_rootIdx, Resource::Handle<D3D12::SRV> a_srvHandle)
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
		auto _pCmd = D3D12::D3D12Wrapper::Instance().GetCommandList4();
		_pCmd->SetComputeRootDescriptorTable(
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
		auto _pCmd = D3D12::D3D12Wrapper::Instance().GetCommandList4();
		_pCmd->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_copyHeap.GetGPU(_startIdx)
		);
	}

	void RenderContext::BindUAVBindLess(UINT a_rootIdx, Resource::Handle<D3D12::UAV> a_handle)
	{
		// コマンドリストにバインド
		auto _pCmd = D3D12::D3D12Wrapper::Instance().GetCommandList4();
		_pCmd->SetComputeRootDescriptorTable(
			a_rootIdx,
			m_bindLessHeap.GetGPU(a_handle.idx)
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

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetGPUHandleBindLess(Resource::Handle<D3D12::SRV> a_handle)
	{
		return m_bindLessHeap.GetGPU(a_handle.idx);
	}

	void RenderContext::ClearRenderTarget(const Resource::Handle<Resource::Texture>& a_texHandle)
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
		D3D12::D3D12Wrapper::Instance().ClearRenderTargetView(_cpu,_tex->GetClearColor());
	}


	void RenderContext::ClearDSV(const Resource::Handle<D3D12::DSV>& a_DSVHandle)
	{
		auto _cpu = D3D12::DescriptorHeapManager::Instance().GetCPU(a_DSVHandle);
		D3D12::D3D12Wrapper::Instance().ClearDepthStencilView(_cpu);
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
		m_pCmdList->Get4()->SetDescriptorHeaps(std::size(_heaps), _heaps);
	}

	void RenderContext::BindCopyHeapAndSumpler()
	{
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
			m_copyHeap.GetHeap(),
			D3D12::DescriptorHeapManager::Instance().RefSamplerHeap()
		};
		m_pCmdList->Get4()->SetDescriptorHeaps(std::size(_heaps), _heaps);
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
		m_pCmdList->Get4()->SetDescriptorHeaps(std::size(_heaps), _heaps);
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
		m_pCmdList->Get4()->Dispatch(a_x,a_y,a_z);
	}



	void RenderContext::UpdateBuffer(
		const std::vector<InstanceData>& a_instanceVec, 
		const std::vector<SubSetData>& a_subsetVec
	)
	{
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetCmdList();

		// インスタンスデータバッファ
		if(!a_instanceVec.empty())
		{
			m_instanceBuffer.UpdateData(a_instanceVec.data(), a_instanceVec.size() * sizeof(InstanceData));
			m_instanceBuffer.Update(*_pCmdList);
		}

		// サブセットデータバッファ
		if(!a_subsetVec.empty())
		{
			m_subsetBuffer.UpdateData(a_subsetVec.data(), a_subsetVec.size() * sizeof(SubSetData));
			m_subsetBuffer.Update(*_pCmdList);
		}

		// ボーン行列の更新
		auto& _bonePalleteVec = Animation::AnimationMatrixManager::Instance().GetBoneMatStorage();
		m_boneBuffer.UpdateData(_bonePalleteVec.data(), _bonePalleteVec.size());
		m_boneBuffer.Update(*_pCmdList);
	}

	void RenderContext::BindIndex(UINT a_instanceBufferIndex, UINT a_subsetBufferIndex, UINT a_rootIndex)
	{
		BufferIndexData _indexData = {};
		_indexData.instanceIndex = a_instanceBufferIndex;
		_indexData.subsetIndex = a_subsetBufferIndex;
		// インデックスバインド
		BindCB()->BindAndAttachDataRootCBV<BufferIndexData>(
			m_pCmdList->NGet(),
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

	void RenderContext::SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_pCmdList->SetGraphicsRootSignature(a_pRootSig);
	}



	void RenderContext::SetGraphicPSO(ID3D12PipelineState* a_pPSO)
	{
		m_pCmdList->NGet()->SetPipelineState(a_pPSO);
		// プリミティブトポロジーセット
		m_pCmdList->NGet()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void RenderContext::SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri)
	{
		m_pCmdList->NGet()->IASetPrimitiveTopology(a_pri);
	}

	void RenderContext::BindMaterialSRV(UINT a_index, const Resource::Material* a_pMaterial)
	{
		// SRVの送信
		std::vector<Resource::Handle<Resource::Texture>> _texVec = {};
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

		std::vector<Resource::Handle<Resource::Texture>> _texVec = {};
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
		m_pCmdList->NGet()->DrawIndexedInstanced(
			_faceCount * 3, 1, _faceStart * 3, 0, 0
		);
	}

	void RenderContext::Draw(uint16_t a_meshID, UINT a_subIdx)
	{
		const auto* _pMesh = Resource::ResourceManager::Instance().Accece<Resource::Mesh>(a_meshID);
		if (!_pMesh) return;

		Draw(_pMesh, a_subIdx);
	}

	void RenderContext::Transition(
		ID3D12Resource* a_pResource,
		D3D12_RESOURCE_STATES a_before,
		D3D12_RESOURCE_STATES a_after
	)
	{
		D3D12::D3D12Wrapper::Instance().ResourceBarrier(
			a_pResource,
			a_before,
			a_after
		);
	}

	void RenderContext::ChangeBackBuffer()
	{
		// レンダーターゲットをバックバッファへ切り替える
		D3D12::D3D12Wrapper::Instance().SetBackBuffer();
	}

	void RenderContext::DrawQuad()
	{
		// コマンドリストの取得
		m_pCmdList->NGet()->DrawInstanced(
			3, 1, 0, 0
		);
	}

	void RenderContext::ShapeDraw()
	{
		// フレーム情報
		auto _currentIdx = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();

		// フレーム頂点バッファ更新
		UINT _vertexCount = static_cast<UINT>(m_pShapeDraw->GetVertexVec().size());
		m_shapeVertexBuffer.UpdateData(m_pShapeDraw->GetVertexVec().data(),m_pShapeDraw->GetVertexVec().size() * m_shapeVertexBuffer.GetStrideSize());


		// 頂点バッファ送信
		m_pCmdList->NGet()->IASetVertexBuffers(0, 1, &m_shapeVertexBuffer.GetView());

		// 描画
		m_pCmdList->NGet()->DrawInstanced(_vertexCount, 1, 0, 0);
	}

	RenderContext::RenderContext()
	{}

	RenderContext::~RenderContext()
	{}
}