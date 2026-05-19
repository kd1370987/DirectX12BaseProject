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
		m_pRootSigManager = a_desc.pRootSigMana;
		m_pGraphicsPSOManager = a_desc.pPSOMana;
		m_pShapeDraw = a_desc.pShapeRender;

		// 形状描画用バッファ作成
		m_shapeVertexBuffer.Create(m_pDevice,m_pShapeDraw->GetMaxCount());

		// ルート定数バッファアロケーター
		m_upCBAllocater = std::make_unique<CBAllocater>();
		m_upCBAllocater->RootCBVCreate(
			m_pDevice, a_desc.cbAllocatorMemSize
		);

		// ボーン用バッファ作成
		m_boneBuffer.Create(a_desc.pDevice, *_pCmdList, a_desc.boneElementNum, nullptr);

		// 構造体バッファ作成のためGPU操作を実行
		D3D12::D3D12Wrapper::Instance().CloseAndExecuteComdLists(_pCmdList);

		// コピー戦略用SRVヒープの作成
		m_copyHeap.Create(
			m_pDevice,
			L"CopyHeap",
			D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeapSize(),
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			0
		);
		m_bindLessHeap.Create(
			m_pDevice,
			L"BindLess",
			D3D12::DescriptorHeapManager::Instance().GetCBVSRVUAVHeapSize(),
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			0
		);

		// クワッドポリゴン
		m_spQuadPolygon = std::make_shared<Resource::QuadPolygon>();
		m_spQuadPolygon->Init();

		// cb5
		m_cb5_Ambient.ambientLightColor = { 0.3f,0.3f,0.3f,1.0f };
		m_cb5_Ambient.directionalLightColor = { 10.0f,10.0f,10.0f,1.0f };
		m_cb5_Ambient.directionalLightDir = { -1.0f,-1.0f,-1.0f,0.0f };

		// カメラの初期化
		auto _eyePos = DirectX::XMVectorSet(0.0f, 0.0f, 10.0f, 0.0f);	// 視点の位置
		auto _targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	// 視点を向ける座標
		auto _upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 上方向を表すベクトル
		constexpr float _fovF = 60.0f;
		constexpr auto _fov = DirectX::XMConvertToRadians(_fovF);						// 視野角

		auto _aspect = static_cast<float>(1280) / static_cast<float>(720);		// アスペクト比
		m_aspectRate = _aspect;
		DirectX::XMStoreFloat4(&m_cb0_camera.cameraPosXYZ, _eyePos);
		DirectX::XMStoreFloat4x4(&m_cb0_camera.viewMat, DirectX::XMMatrixLookAtLH(_eyePos, _targetPos, _upward));
		DirectX::XMStoreFloat4x4(&m_cb0_camera.projMat, DirectX::XMMatrixPerspectiveFovLH(_fov, _aspect, 0.3f, 1000.0f));
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
	void RenderContext::SetToShader(
		const DirectX::XMFLOAT4X4& a_worldMat
	)
	{
		// カメラの位置を更新
		m_cb0_camera.cameraPosXYZ = {
			a_worldMat._41,
			a_worldMat._42,
			a_worldMat._43,
			0.0f
		};

		// ビュー行列を計算して格納
		DirectX::XMMATRIX _wMat = DirectX::XMLoadFloat4x4(&a_worldMat);
		DirectX::XMMATRIX _vMat = DirectX::XMMatrixInverse(nullptr, _wMat);
		DirectX::XMStoreFloat4x4(&m_cb0_camera.viewMat, _vMat);
		DirectX::XMStoreFloat4x4(&m_cb0_camera.viewInvMat, _wMat);
	}

	float RenderContext::GetCameraAspectRate()
	{
		return m_aspectRate;
	}

	const DirectX::XMFLOAT4X4& RenderContext::GetCameraRotMat()
	{
		DXSM::Matrix _mat = m_cb0_camera.viewInvMat;
		DXSM::Vector3 _pos;
		DXSM::Quaternion _rot;
		DXSM::Vector3 _scale;

		_mat.Decompose(_scale, _rot, _pos);

		return DXSM::Matrix::CreateFromQuaternion(_rot);
	}

	const DXSM::Vector3& RenderContext::GetCameraPOS()
	{
		DXSM::Vector3 _pos = { m_cb0_camera.cameraPosXYZ.x,m_cb0_camera.cameraPosXYZ.y,m_cb0_camera.cameraPosXYZ.z };
		return _pos;
	}

	void RenderContext::BindCameraCB()
	{
		// カメラ用定数バッファに転送
		BindCB()->BindSemanticCBV<RootSigSemantic::CameraCB>(
			m_pCmdList->NGet(),
			0,
			m_cb0_camera
		);	
	}

	void RenderContext::BindAmbientCB()
	{
		// 環境
		BindCB()->BindSemanticCBV<RootSigSemantic::AmbientCB>(
			m_pCmdList->NGet(),
			1,
			m_cb5_Ambient
		);
		
	}


	void RenderContext::SetProjectionMatrix(DirectX::XMFLOAT4X4 a_projMat)
	{
		m_cb0_camera.projMat = a_projMat;
	}

	CBAllocater* RenderContext::BindCB()
	{
		return m_upCBAllocater.get();
	}

	void RenderContext::BindRootCBV(UINT a_index, const void* a_pData, size_t a_size)
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
			//m_bindLessHeap.GetGPU(a_handle.idx + 200)
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
		m_pDevice->CopyDescriptorsSimple(
			300,
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

	void RenderContext::BindCBBone(const Storage::Range & a_range)
	{
		// ボーンのバインド
		m_cb4_Bone = {};
		m_cb4_Bone.startIdx = a_range.startIndex;
		m_cb4_Bone.count = a_range.rangeSize;

		BindCB()->BindAndAttachDataRootCBV<CBBone>(
			m_pCmdList->NGet(),
			4,
			m_cb4_Bone
		);
	}


	void RenderContext::AddItem(const RenderQueueType& a_type, const DrawItem& a_item)
	{
		m_drawItemMap[a_type].push_back(a_item);
	}

	void RenderContext::AddItem(RenderQueueType2D a_type, const DrawItem2D& a_itemVec)
	{
		m_drawItem2DMap[a_type].push_back(a_itemVec);
	}

	std::span<const DrawItem> RenderContext::GetItemVec(const RenderQueueType& a_type) const
	{
		auto _it = m_drawItemMap.find(a_type);
		if (_it != m_drawItemMap.end())
		{
			return _it->second;
		}
		std::vector<DrawItem> _items = {};
		return _items;
	}

	const std::vector<DrawItem2D>& RenderContext::GetItemVec(const RenderQueueType2D& a_type) const
	{
		auto _it = m_drawItem2DMap.find(a_type);
		if (_it != m_drawItem2DMap.end())
		{
			return _it->second;
		}
		return {};
	}

	void RenderContext::Excute(RenderGraph* a_pGraph)
	{
		// ボーン行列の更新
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetCmdList();
		auto& _bonePalleteVec = Animation::AnimationMatrixManager::Instance().GetBoneMatStorage();
		m_boneBuffer.UpdateData(_bonePalleteVec.data(),_bonePalleteVec.size());
		m_boneBuffer.Update(*_pCmdList);

		// バインド対象のクリア
		m_currentRootSigID = Resource::Limits::INVALID_ID;
		m_currentPSOID = Resource::Handle<D3D12::PipelineState>();
		m_pCurrentMaterial = nullptr;
		m_pCurrentMesh = nullptr;
		m_pCurrentPoly = nullptr;

		// レンダーパスの実行
		a_pGraph->Excute(this);

		// 描画対象アイテムリストのクリア
		m_drawItemMap.clear();
		m_drawItem2DMap.clear();
	}

	void RenderContext::ClearCmd()
	{
		// 描画対象アイテムリストのクリア
		m_drawItemMap.clear();
		m_drawItem2DMap.clear();
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


	void RenderContext::BindObuje(UINT a_index, const DirectX::XMFLOAT2& a_uv, const DirectX::XMFLOAT2& a_tile)
	{
		m_cb1_object.uvOffsetTiling.x = a_uv.x;
		m_cb1_object.uvOffsetTiling.y = a_uv.y;
		m_cb1_object.uvOffsetTiling.z = a_tile.x;
		m_cb1_object.uvOffsetTiling.w = a_tile.y;

		BindCB()->BindAndAttachDataRootCBV<CBObject>(
			m_pCmdList->NGet(),
			1,
			m_cb1_object
		);
	}


	void RenderContext::BindMaterial(UINT a_index, const Resource::Material* a_pMaterial, const DirectX::XMFLOAT4& a_colorScale, const DirectX::XMFLOAT3& a_emissiveScale)
	{
		// ベースカラー
		DXSM::Vector4 _colorScale(a_colorScale);
		DXSM::Vector4 _materialScale(a_pMaterial->baseColor);
		m_cb3_Material.baseColorXYZW = _materialScale * _colorScale;

		// エミッシブ
		DXSM::Vector3 _emissiveScale(a_emissiveScale);
		DXSM::Vector3 _materialEmissiveScale(a_pMaterial->emissive);
		DXSM::Vector3 _emiVec3 = _materialEmissiveScale * _emissiveScale;
		m_cb3_Material.emissiveXYZ = { _emiVec3.x,_emiVec3.y,_emiVec3.z,1 };

		// マテリアルラフネス
		m_cb3_Material.metallicRoughnessXY = { a_pMaterial->metallic ,a_pMaterial->roughness,0,0 };

		// マテリアルバッファバインド
		BindCB()->BindAndAttachDataRootCBV<CBMaterial>(
			m_pCmdList->NGet(),
			a_index,
			m_cb3_Material
		);
	}

	void RenderContext::BindMaterialSRV(UINT a_index, const Resource::Material* a_pMaterial)
	{
		// SRVの送信
		if (a_pMaterial != m_pCurrentMaterial)
		{
			std::vector<Resource::Handle<Resource::Texture>> _texVec = {};
			_texVec.push_back(a_pMaterial->baseColorTex);
			_texVec.push_back(a_pMaterial->metaRoughTex);
			_texVec.push_back(a_pMaterial->emissiveTex);
			_texVec.push_back(a_pMaterial->normalTex);
			BindSRV(a_index, _texVec);
		}
	}


	void RenderContext::BindMesh(UINT a_index, Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat)
	{
		// メッシュ変換行列の転送
		m_cb2_MeshTrans.worldMat = a_worldMat;
		BindCB()->BindAndAttachDataRootCBV<CBMeshTrans>(
			m_pCmdList->NGet(),
			a_index,
			m_cb2_MeshTrans
		);

		// 頂点バッファとインデックスバッファのセット
		//if (a_pMesh != m_pCurrentMesh)
		{
			if (!a_pMesh) return;
			//auto _vertView = a_pMesh->GetVertexBuffer().GetView();
			if (!a_pMesh->HasRasterData()) return;
			auto _vertView = a_pMesh->GetRasterData().vertexBuffer.GetView();
			m_pCmdList->IASetVertexBuffers(0, 1, &_vertView);
			const auto& _pIndexView = a_pMesh->GetRasterData().indexBuffer.GetView();
			m_pCmdList->IASetIndexBuffer(&_pIndexView);
		}
	}

	void RenderContext::Draw(Resource::Mesh* a_pMesh, UINT a_subIdx)
	{
		// 描画
		UINT _faceCount = static_cast<UINT>(a_pMesh->GetMetaData().subsets[a_subIdx].faceCount);
		UINT _faceStart = static_cast<UINT>(a_pMesh->GetMetaData().subsets[a_subIdx].faceStart);
		m_pCmdList->NGet()->DrawIndexedInstanced(
			_faceCount * 3, 1, _faceStart * 3, 0, 0
		);
	}

	void RenderContext::SetViewPort()
	{}

	void RenderContext::SetScissorRect()
	{}

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



	void RenderContext::DrawUIQueue(RenderQueueType2D a_type)
	{
		// 描画アイテム取得
		auto& _draws = GetItemVec(a_type);
		if (_draws.size() == 0) return;
		for (auto& _item : _draws)
		{
			m_cbUI.uiMat = _item.worldMat;
			m_cbUI.color = _item.colorScale;

			BindCB()->BindAndAttachDataRootCBV<CBUI>(
				m_pCmdList->NGet(),
				0,
				m_cbUI
			);

			// SRVの送信
			auto _handle = D3D12::DescriptorHeapManager::Instance().GetCPU(_item.srvHandleRange);
			BindSRV(0, _handle);


			// メッシュ変換行列の転送
			if (m_pCurrentPoly != m_spQuadPolygon.get())
			{
				m_pCmdList->NGet()->IASetVertexBuffers(0, 1, &m_spQuadPolygon->GetVBView());
				m_pCmdList->NGet()->IASetIndexBuffer(&m_spQuadPolygon->GetIBView());

				m_pCurrentPoly = m_spQuadPolygon.get();
			}

			// 描画
			m_pCmdList->NGet()->DrawIndexedInstanced(
				6, 1, 0, 0, 0
			);
		}
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
	{
		m_currentPSOID = Resource::Handle<D3D12::PipelineState>();
		m_currentRootSigID = Resource::Limits::INVALID_ID;
	}

	RenderContext::~RenderContext()
	{}
}