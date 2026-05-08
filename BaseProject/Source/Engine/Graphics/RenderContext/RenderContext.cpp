#include "RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/ConstantBuffer/ConstantBuffer.h"

#include "Engine/D3D12//D3DObject/CommandList/CommandList.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"


#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"

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
		// デバイスのキャッシュ
		m_pDevice = a_desc.pDevice;

		// ポインタのキャッシュ
		m_pShaderManger = a_desc.pShaderMana;
		m_pRootSigManager = a_desc.pRootSigMana;
		m_pGraphicsPSOManager = a_desc.pPSOMana;
		m_pShapeDraw = a_desc.pShapeRender;

		// 形状描画用バッファ作成
		m_shapeVertexBuffer.Create(
			m_pShapeDraw->GetMaxCount(),
			sizeof(Vertex),
			nullptr
		);

		// ルート定数バッファアロケーター
		m_upCBAllocater = std::make_unique<CBAllocater>();
		m_upCBAllocater->RootCBVCreate(
			m_pDevice, a_desc.cbAllocatorMemSize
		);

		// コピー戦略用SRVヒープの作成
		m_copyHeap.Create(
			m_pDevice,
			L"CopyHeap",
			300,
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
		// レジスター番号取得
		UINT _regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::CameraCB);


		// ルートシグネチャにカメラCBが含まれているのなら
		if (D3D12::ERR_UINT != _regiIdx)
		{
			// カメラ用定数バッファに転送
			BindCB()->BindSemanticCBV<RootSigSemantic::CameraCB>(
				m_pCmdList->NGet(),
				_regiIdx,
				m_cb0_camera
			);
		}

		// 環境
		_regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::AmbientCB);
		if (D3D12::ERR_UINT != _regiIdx)
		{
			BindCB()->BindSemanticCBV<RootSigSemantic::AmbientCB>(
				m_pCmdList->NGet(),
				_regiIdx,
				m_cb5_Ambient
			);
		}
	}


	void RenderContext::SetProjectionMatrix(DirectX::XMFLOAT4X4 a_projMat)
	{
		m_cb0_camera.projMat = a_projMat;
	}

	CBAllocater* RenderContext::BindCB()
	{
		return m_upCBAllocater.get();
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

	void RenderContext::BindSRV(
		RootSigSemantic a_sema,
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_srvHandle
	)
	{
		UINT _regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, a_sema);


		// SRVセット
		if (D3D12::ERR_UINT != _regiIdx)
		{
			BindSRV(_regiIdx,a_srvHandle);
		}
	}

	void RenderContext::BindSRV(RootSigSemantic a_sema, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle)
	{
		UINT _regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, a_sema);

		// SRVセット
		if (D3D12::ERR_UINT != _regiIdx)
		{
			BindSRV(_regiIdx, a_cpuHandle);
		}
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
		//m_pCmdList->NGet()->SetGraphicsRootDescriptorTable(
		//	a_rootIdx,
		//	m_copyHeap.GetGPU(_startIdx)
		//);
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

	void RenderContext::ClearRenderTarget(const Resource::Handle<Resource::Texture>& a_texHandle)
	{
		//auto& _tex = Resource::TextureManager::Instance().RefTexture(a_texHandle);
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

	void RenderContext::BindHeaps(UINT a_numHeaps, ID3D12DescriptorHeap* const* a_pHeaps)
	{
		m_pCmdList->SetDescriptorHeaps(a_numHeaps,a_pHeaps);
	}


	void RenderContext::AddItem(const RenderQueueType& a_type, const DrawItem& a_item)
	{
		m_drawItemMap[a_type].push_back(a_item);
	}

	void RenderContext::AddItem(RenderQueueType2D a_type, const DrawItem2D& a_itemVec)
	{
		m_drawItem2DMap[a_type].push_back(a_itemVec);
	}

	const std::vector<DrawItem>& RenderContext::GetItemVec(const RenderQueueType& a_type) const
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
		std::vector<DrawItem2D> _items = {};
		return _items;
	}

	void RenderContext::Excute(RenderGraph* a_pGraph)
	{
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


	void RenderContext::SetGraphicsRootSignature(const Resource::ID& a_rootSigID)
	{
		// ルートシグネチャセット
		if (a_rootSigID != m_currentRootSigID)
		{
			//m_pCmdList->NGet()->SetGraphicsRootSignature(m_pRootSigManager->NGet(a_rootSigID));
			m_pCmdList->SetGraphicsRootSignature(m_pRootSigManager->NGet(a_rootSigID));
			m_currentRootSigID = a_rootSigID;
		}
	}

	void RenderContext::SetGraphicPSO(const Resource::Handle<D3D12::PipelineState>& a_handle)
	{
		// パイプラインステートセット
		if (a_handle != m_currentPSOID)
		{
			m_pCmdList->NGet()->SetPipelineState(m_pGraphicsPSOManager->Ref(a_handle));
			m_currentPSOID = a_handle;
		}

		// プリミティブトポロジーセット
		m_pCmdList->NGet()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void RenderContext::BindObuje(const DirectX::XMFLOAT2& a_uv, const DirectX::XMFLOAT2& a_tile)
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

	void RenderContext::BindMaterial(
		const Resource::Material* a_pMaterial,
		const DirectX::XMFLOAT4& a_colorScale,
		const DirectX::XMFLOAT3& a_emissiveScale
	)
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
			3,
			m_cb3_Material
		);

		// SRVの送信
		UINT _regiIdx =m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::MaterialSRV);
		if (a_pMaterial != m_pCurrentMaterial)
		{
			std::vector<Resource::Handle<Resource::Texture>> _texVec = {};
			_texVec.push_back(a_pMaterial->baseColorTex);
			_texVec.push_back(a_pMaterial->metaRoughTex);
			_texVec.push_back(a_pMaterial->emissiveTex);
			_texVec.push_back(a_pMaterial->normalTex);
			BindSRV(_regiIdx, _texVec);
		}
	}

	void RenderContext::BindMesh(Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat)
	{
		// メッシュ変換行列の転送
		m_cb2_MeshTrans.worldMat = a_worldMat;
		UINT _regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::MeshTransCB);
		assert(_regiIdx != UINT_MAX);
		BindCB()->BindAndAttachDataRootCBV<CBMeshTrans>(
			m_pCmdList->NGet(),
			_regiIdx,
			m_cb2_MeshTrans
		);

		if (a_pMesh != m_pCurrentMesh)
		{
			if (!a_pMesh) return;
			m_pCmdList->IASetVertexBuffers(0,1,&a_pMesh->GetVertexBuffer().View());
			m_pCmdList->IASetIndexBuffer(&a_pMesh->GetIndexBuffer().View());
			m_pCurrentMesh = a_pMesh;
		}
	}

	void RenderContext::BindIndex(const DXSM::Vector4& a_vec4)
	{
		CBMaterialIndex _s = {};
		_s.indexXYZW = a_vec4;
		UINT _regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::MaterialIndexCB);
		BindCB()->BindAndAttachDataRootCBV<CBMaterialIndex>(
			m_pCmdList->NGet(),
			_regiIdx,
			_s
		);
	}


	void RenderContext::BindBone(const DirectX::XMFLOAT4X4* a_pMatVec, UINT a_count)
	{
		// 定数バッファにコピー
		if (a_pMatVec)
		{
			std::memcpy(m_cb4_Bone.boneMat, a_pMatVec, sizeof(DirectX::XMFLOAT4X4) * a_count);
		}

		// ルートパラムインデックス確保
		UINT _regiIdx =
			m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::BoneCB);
		
		// バッファにコピー
		BindCB()->BindAndAttachDataRootCBV<CBBone>(
			m_pCmdList->NGet(),
			_regiIdx,
			m_cb4_Bone
		);
	}

	void RenderContext::Draw(Resource::Mesh* a_pMesh, UINT a_subIdx)
	{
		// 描画
		UINT _faceCount = static_cast<UINT>(a_pMesh->GetSubsets()[a_subIdx].faceCount);
		UINT _faceStart = static_cast<UINT>(a_pMesh->GetSubsets()[a_subIdx].faceStart);
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

	void RenderContext::DrawQueue(RenderPassID a_passID, LightingType a_lightingType)
	{
		for (auto& _item : m_drawItemVec)
		{
			if (_item.passID != a_passID) continue;
			if (_item.lightingType != a_lightingType) continue;

			BindObuje(
				{ 0.0f,0.0f },
				{ 1.0f,1.0f }
			);

			BindMaterial(_item.pMaterial, _item.colorScale, _item.emissiveScale);
			BindMesh(_item.pMesh, _item.worldMat);

			// ボーン行列があるのなら転送
			if (_item.pBoneMatrices)
			{
				BindBone(
					_item.pBoneMatrices,
					_item.boneCount
				);
			}

			Draw(_item.pMesh, _item.subIdx);
		}
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
			UINT _regiIdx =m_pRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::MaterialSRV);
			auto _handle = D3D12::DescriptorHeapManager::Instance().GetCPU(_item.srvHandleRange);
			BindSRV(_regiIdx, _handle);


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

	D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetImGuiGPUHandle(const std::string& a_name)
	{
		//return m_upRenderGraph->GetImGuiGPUHandle(a_name);
		return D3D12_GPU_DESCRIPTOR_HANDLE();
	}

	void RenderContext::SetPrimitive(D3D_PRIMITIVE_TOPOLOGY a_topology)
	{
		// プリミティブトポロジーセット
		m_pCmdList->NGet()->IASetPrimitiveTopology(a_topology);
	}

	void RenderContext::SetRasterizerFillMode(D3D12_FILL_MODE a_fillMode)
	{

	}

	void RenderContext::ShapeDraw()
	{
		// フレーム情報
		auto _currentIdx = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();

		// フレーム頂点バッファ更新
		UINT _vertexCount = static_cast<UINT>(m_pShapeDraw->GetVertexVec().size());
		m_shapeVertexBuffer.Update(_vertexCount,m_pShapeDraw->GetVertexVec().data());

		// 頂点バッファ送信
		m_pCmdList->NGet()->IASetVertexBuffers(0, 1, &m_shapeVertexBuffer.View());

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