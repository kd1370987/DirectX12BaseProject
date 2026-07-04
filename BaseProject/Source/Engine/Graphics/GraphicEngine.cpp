#include "GraphicEngine.h"

#include "../MainEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../D3D12/PipelineStateManager/PipelineStateManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "RenderContext/ShapeDraw/ShapeDraw.h"
#include "RenderGraph/RenderGraph.h"
#include "../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../Particle/ParticleBufferManager.h"
#include "RenderPassRegistry/RenderPassRegistry.h"
#include "MeshBufferAllocator/MeshBufferAllocator.h"

// オプション
#include "../Option/OptionManager.h"

// レンダーパス
#include "RenderPass/RasterizePass/ZPrePass/ZPrePass.h"
#include "RenderPass/RasterizePass/ParticlePass/ParticlePass.h"
#include "RenderPass/GBufferPass/GBufferPass.h"
#include "RenderPass/RasterizePass/FullScreenPass/FullScreenPass.h"
#include "RenderPass/RasterizePass/DebugLinePass/DebugLinePass.h"

#include "RenderPass/RaytracingPass/FullRaytracingPass/FullRaytracingPass.h"
#include "RenderPass/RaytracingPass/RaytracingGIPass/RaytracingGIPass.h"
#include "RenderPass/RaytracingPass/RaytracingShadowPass/RaytracingShadowPass.h"

#include "RenderPass/ComputePass/Lighting/DeferredLighting/DeferredLighting.h"
#include "RenderPass/CopyPass/GBufferHistoryPass/GBufferHistoryPass.h"
#include "RenderPass/CopyPass/PostHistoryPass/PostHistoryPass.h"
#include "RenderPass/ComputePass/Denoise/TempralAccumulationPass/TemporalAccumulationPass.h"
#include "RenderPass/ComputePass/Denoise/GI/GISpatialDenoisePass/GISpatialDenoisePass.h"
#include "RenderPass/ComputePass/AntiAliasing/TAA/TAAPass.h"
#include "RenderPass/ComputePass/Denoise/Shadow/ShadowTemporalAccumulationPass/ShadowTemporalAccumulationPass.h"
#include "RenderPass/ComputePass/Effect/Particle/EmitParticlePass/EmitParticlePass.h"
#include "RenderPass/ComputePass/Effect/Particle/UpdateParticlePass.h"

#include "../ECS/World/World.h"

#include "RenderPass/MeshShaderPass/TestMeshPass/TestMeshPass.h"

namespace Engine::Graphics
{
	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(
		D3D12::GraphicsCommandList* a_pCmdList,
		const GraphicsEngineDesc& a_desc
	)
	{

		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

		m_pPipelineStateManager = a_desc.pPipelineStateManager;

		// 形状描画クラス構築
		m_upShapeRender = std::make_unique<ShapeRenderer>();

		// メッシュバッファ管理クラス
		m_upMeshBufferAllocator = std::make_unique<MeshBufferAllocator>();
		m_upMeshBufferAllocator->Init(
			_pDevice,
			a_pCmdList,
			5000000,
			200000,
			5000000,
			10000000
		);

		// レンダーコンテキストの作成
		for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			auto _upCtx = std::make_unique<RenderContext>();

			RenderContextDesc _desc = {};
			_desc.pDevice = _pDevice;
			_desc.pShapeRender = m_upShapeRender.get();

			_desc.cbAllocatorMemSize = 32 * 1024 * 1024;
			_desc.boneElementNum = 10000;

			_upCtx->Init(this, a_pCmdList, _desc);
			m_upRenderContextVec.push_back(std::move(_upCtx));
		}

		// レンダーパスの登録
		m_upRenderPassRegistry = std::make_unique<RenderPassRegistry>();
		// ラスター関係
		AddZPrePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Setup);
		AddGBufferPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Geometry);
		//AddMeshGBufferPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Geometry);
		AddDebugLinePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::UI);
		AddFullScreenPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Present);

		AddFullRaytracingPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Geometry);
		AddRaytracingShadowPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Shadow);
		AddRaytracingGIPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Raytracing);
		AddDeferredLighting(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Lighting);
		AddGBufferHistoryPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::HistoryUpdate);
		AddPostHistoryPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::HistoryUpdate);

		AddTAAPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);

		AddMSTestPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::UI);

		AddShadowTemporalAccumulationPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);

		AddTemporalAccumulationPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);
		AddGISpatialDenoisePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);

		// パーティクル
		AddEmitParticlePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Particle);
		AddUpdateParticlePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Particle);
		AddParticlePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Particle);

		// レンダーグラフの構築
		m_upRenderGraph = std::make_unique<RenderGraph>();
		m_upRenderGraph->Init(m_upRenderPassRegistry.get());

		// 定数バッファ初期化
		m_cbAmbient = {};
		m_cbAmbient.ammbientColorScale = { 0,0,0 };
		m_cbAmbient.dlDir = { 0.5f,-1.0f,0.5f };
		m_cbAmbient.dlColor = { 4.0f,4.0f,4.0f };
	}

	void GraphicsEngine::Release()
	{
		// レンダーコンテキスト解放
		for (auto& _ctx : m_upRenderContextVec)
		{
			_ctx->Release();
			_ctx.reset();
		}

		// 各リンク解除
		m_pPipelineStateManager = nullptr;


		m_upMeshBufferAllocator->Release();

	}

	void GraphicsEngine::BegineFrame()
	{
		// 今から使うレンダーコンテキスをクリア
		m_currentFrameIndex = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();
		m_upRenderContextVec[m_currentFrameIndex]->Clear();
	}
	void GraphicsEngine::Excute()
	{
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();
		auto _currentFence = D3D12::D3D12Wrapper::Instance().GetCurrentFenceValue();

		// メッシュバッファの更新
		m_upMeshBufferAllocator->UpdateFrame(_pCmdList, _currentFence);

		// パーティクルのバッファ更新
		MainEngine::Instance().RefParticleManager()->UploadEmitData(_pCmdList);

		// バックバッファのresourceバリア
		D3D12::ResourceBarrier(
			_pCmdList, // D3D12Wrapper側のバリア関数も引数でリストをもらうように修正してください
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffar(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffarTex().GetRTV()
		);
		float _clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f }; // 背景色
		_pCmdList->ClearRenderTargetView(_cpuHandle, _clearColor, 0, nullptr);

		// レンダーコンテキストにコマンドリストをセット
		m_upRenderContextVec[m_currentFrameIndex]->SetDirectCommandList(_pCmdList);

		// GPU用カメラデータを作成
		CreateGPUCameraData();

		// バッファの更新
		m_upRenderContextVec[m_currentFrameIndex]->UpdateBuffer(m_instanceDataVec, m_subSetDataVec,m_meshInstanceDataVec,m_meshMaterialDataVec);

		// 描画アイテムをソート
		std::sort(
			m_lightWeightDrawItemVec.begin(), m_lightWeightDrawItemVec.end(),
			[](const LightWeightDrawItem& a, const LightWeightDrawItem& b)
			{
				return a.sortKey.value < b.sortKey.value;
			}
		);

		// レンダーパス実行
		m_upRenderGraph->Excute(this,m_upRenderContextVec[m_currentFrameIndex].get());

		D3D12::D3D12Wrapper::Instance().SubmitDirectCommandList(_pCmdList);
		m_upRenderContextVec[m_currentFrameIndex]->SetDirectCommandList(nullptr);

	}
	void GraphicsEngine::EndFrame()
	{
		// 描画命令をクリアしてメモリ領域を確保しておく
		m_lightWeightDrawItemVec.clear();
		m_lightWeightDrawItemVec.reserve(10000);

		// オブジェクトデータの消去
		m_instanceDataVec.clear();
		m_instanceDataVec.reserve(10000);
		m_meshInstanceDataVec.clear();
		m_meshInstanceDataVec.reserve(10000);

		// サブセット情報の消去
		m_subSetDataVec.clear();
		m_subSetDataVec.reserve(10000);
		m_meshMaterialDataVec.clear();
		m_meshMaterialDataVec.reserve(10000);

		
		// デバッグ用配列のクリア
		Editor::MainEditor::Instance().ClearBuffer();
	}

	const Graphics::RenderContext* GraphicsEngine::GetRenderContext() const
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
	}
	Graphics::RenderContext* GraphicsEngine::RefRenderContext()
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
		
	}
	D3D12::PipelineStateManager* GraphicsEngine::RefPipelineStateManager()
	{
		return m_pPipelineStateManager;
	}
	Graphics::RenderPassRegistry* GraphicsEngine::RefRenderPassRegistry()
	{
		return m_upRenderPassRegistry.get();
	}
	RenderGraph* GraphicsEngine::RefRenderGraph()
	{
		return m_upRenderGraph.get();
	}
	MeshAllocationHandle GraphicsEngine::AllocateAndUpload(D3D12::GraphicsCommandList* a_pCmdList, const Resource::Mesh& a_newMeshData)
	{
		return m_upMeshBufferAllocator->AllocateAndUpload(a_pCmdList,a_newMeshData);
	}
	void GraphicsEngine::Free(const MeshAllocationHandle& a_handle)
	{
		m_upMeshBufferAllocator->Free(a_handle, D3D12::D3D12Wrapper::Instance().GetCurrentFenceValue());
	}
	void GraphicsEngine::SetCameraMat(const DXSM::Matrix& a_worldMat)
	{
		// 座標を代入
		m_cbCamera.pos = { a_worldMat._41,a_worldMat._42,a_worldMat._43 ,1};

		// ビュー行列・逆ビュー行列をセット
		m_cbCamera.viewMat = a_worldMat.Invert();
		m_cbCamera.viewInvMat = a_worldMat;
	}
	void GraphicsEngine::SetProjMat(const DXSM::Matrix & a_projMat)
	{
		m_cbCamera.projMat = a_projMat;
		m_cbCamera.projInvMat = a_projMat.Invert();
	}
	void GraphicsEngine::BindMeshBuffer(D3D12::GraphicsCommandList* a_pCmdList)
	{
		m_upMeshBufferAllocator->BindBuffers(a_pCmdList);
	}
	const CameraData& GraphicsEngine::GetCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetGPUCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetCPUCameraData() const
	{
		return m_cbCamera;
	}
	void GraphicsEngine::SetAmbientData(const AmbientData& a_data)
	{
		m_cbAmbient = a_data;
	}
	const AmbientData& GraphicsEngine::GetAmbientData() const
	{
		return m_cbAmbient;
	}
	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Color& a_albedScale, 
		const DXSM::Vector3& a_emissiveScale
	)
	{
		SubmitModel(
			a_world,
			a_pModel,
			a_worldMatrix,
			a_worldMatrix,
			a_albedScale,
			a_emissiveScale
		);
	}
	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel, 
		const DXSM::Matrix& a_worldMatrix, 
		const DXSM::Matrix& a_prevMatrix,
		const DXSM::Color& a_albedScale,
		const DXSM::Vector3& a_emissiveScale
	)
	{
		uint8_t _zpreIdx = m_upRenderGraph->GetPassIndex("ZPre");
		uint8_t _opeqIdx = m_upRenderGraph->GetPassIndex("GBuffer");
		const uint8_t _zpreStatic = m_upRenderGraph->GetPass("ZPre")->GetPSOIndex("ZPreStatic");
		const uint8_t _gbuffStatic = m_upRenderGraph->GetPass("GBuffer")->GetPSOIndex("GBufferStatic");

		// モデルが持っている描画コマンド（サブセット）を展開
		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();
		for (const auto& _cmd : _drawCmdVec)
		{
			auto* _pMaterial = Engine::Resource::ResourceManager::Instance().Accece<Engine::Resource::Material>(_cmd.materialRawID);
			if (!_pMaterial) continue;

			// ノードの行列確定
			DXSM::Matrix _nodeTransMat(a_pModel->GetOriginalNodeVec()[_cmd.nodeIndex].worldTransform);
			DXSM::Matrix _mat = _nodeTransMat * a_worldMatrix;
			DXSM::Matrix _prevMat = _nodeTransMat * a_prevMatrix;

			// -----------------------------------------------------
			// GPU用データの構築
			// -----------------------------------------------------
			Engine::Graphics::InstanceData _instanceData = {};
			_instanceData.worldMat = _mat.Transpose();
			_instanceData.prevWorldMat = _prevMat.Transpose();

			Engine::Graphics::SubSetData _subSetData = {};
			_subSetData.baseColorScale = a_albedScale;
			_subSetData.emissiveColorScale = a_emissiveScale;
			_subSetData.metallic = _pMaterial->metallic;
			_subSetData.roughness = _pMaterial->roughness;

			// -----------------------------------------------------
			// メッシュシェーダー用パス
			// -----------------------------------------------------
			//Engine::Graphics::MeshInstanceData _meshInstance = {};
			//Engine::Graphics::MeshMaterial _meshMaterial = {};
			//// メッシュマテリアル
			//if (_cmd.pMesh->HasMeshShaderData())
			//{
			//	_meshMaterial.baseColor = _modelComp.colorScale; // マテリアルの色も入れろ
			//	_meshMaterial.emissive = _modelComp.emissiveScale;
			//	_meshMaterial.metallic = 0.1f;
			//	_meshMaterial.roughness = 0.5f;
			//	_meshMaterial.albedIndex = _cmd.pMaterial->baseColorTex.GetIndex();
			//	_meshMaterial.metaRoughnessIndex = _cmd.pMaterial->metaRoughTex.GetIndex();
			//	_meshMaterial.emissiveIndex = _cmd.pMaterial->emissiveTex.GetIndex();
			//	_meshMaterial.normalIndex = _cmd.pMaterial->normalTex.GetIndex();

			//	// メッシュデータ作成
			//	_meshInstance.worldMat = _mat.Transpose();
			//	_meshInstance.prevWorldMat = _mat.Transpose();
			//	_meshInstance.boneStartIndex = 0;
			//	_meshInstance.boneCount = 0;
			//	_meshInstance.materialOffset = _pGE->SetMeshMaterialData(_meshMaterial);
			//	_meshInstance.meshletOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.meshletHandle.startIndex;
			//	_meshInstance.vertexOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.vertexHandle.startIndex;
			//	_meshInstance.uviOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.uniqueVertexHandle.startIndex;
			//	_meshInstance.primitiveOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.primitiveHandle.startIndex;
			//	//_item.subsetMeshletCount = _msData.meshHandle.meshletHandle.startIndex + _subsetMeshletInfo.meshletOffset;
			//	_item.meshHandle = _cmd.pMesh->GetMeshShaderData().meshHandle;
			//}

			// -----------------------------------------------------
			// 描画アイテムの登録
			// -----------------------------------------------------
			Engine::Graphics::LightWeightDrawItem _item = {};
			_item.sortKey.bits.meshID = _cmd.meshRawID;
			_item.sortKey.bits.materialID = _cmd.materialRawID;
			_item.subIndex = _cmd.subIdx;

			_item.instnaceIndex = SetInstanceData(_instanceData);
			_item.subsetIndex = SetSubSetData(_subSetData);

			switch (_cmd.alphaMode)
			{
			case Engine::Resource::Alpha::Opaque:
				_item.sortKey.bits.psoID = _zpreStatic;
				_item.sortKey.bits.passIndex = _zpreIdx;
				AddItem(_item);
				_item.sortKey.bits.psoID = _gbuffStatic;
				_item.sortKey.bits.passIndex = _opeqIdx;
				AddItem(_item);
				break;
			case Engine::Resource::Alpha::Mask:
				_item.sortKey.bits.psoID = _zpreStatic;
				_item.sortKey.bits.passIndex = _zpreIdx;
				AddItem(_item);
				_item.sortKey.bits.psoID = _gbuffStatic;
				_item.sortKey.bits.passIndex = _opeqIdx;
				AddItem(_item);
				break;
			case Engine::Resource::Alpha::Blend:
				break;
			default:
				break;
			}
		}
	}
	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix, 
		const DXSM::Matrix& a_prevMatrix, 
		const RangeHandle<Resource::BoneMatrix>& a_boneHandle, 
		const RangeHandle<Resource::NodePoseMatrix>& a_nodePoseHandle,
		const DXSM::Color& a_albedScale, 
		const DXSM::Vector3& a_emissiveScale
	)
	{
		// パス取得 : これは絶対にやめるマテリアルに持たせるべき
		uint8_t _zpreIdx = m_upRenderGraph->GetPassIndex("ZPre");
		uint8_t _opeqIdx = m_upRenderGraph->GetPassIndex("GBuffer");
		const uint8_t _zpreStatic = m_upRenderGraph->GetPass("ZPre")->GetPSOIndex("ZPreAnimation");
		const uint8_t _gbuffStatic = m_upRenderGraph->GetPass("GBuffer")->GetPSOIndex("GBufferAnimation");

		// ノード行列取得
		auto& _nodePosePool = a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
		const auto& _nodePoseMatVec = _nodePosePool.GetRange(a_nodePoseHandle);

		// モデルが持っている描画コマンド（サブセット）を展開
		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();
		for (const auto& _cmd : _drawCmdVec)
		{
			auto* _pMaterial = Engine::Resource::ResourceManager::Instance().Accece<Engine::Resource::Material>(_cmd.materialRawID);
			if (!_pMaterial) continue;

			// ノードのワールド行列を確定
			DXSM::Matrix _nodeTransMat(_nodePoseMatVec[_cmd.nodeIndex].world);
			DXSM::Matrix _mat = _nodeTransMat * a_worldMatrix;

			// -----------------------------------------------------
			// GPU用データの構築
			// -----------------------------------------------------
			InstanceData _instanceData = {};
			_instanceData.worldMat = _mat.Transpose();
			_instanceData.prevWorldMat = _mat.Transpose();
			_instanceData.boneStartIndex = a_boneHandle.startIndex;
			_instanceData.boneCount = a_boneHandle.count;

			SubSetData _subSetData = {};
			_subSetData.baseColorScale = a_albedScale;
			_subSetData.emissiveColorScale = a_emissiveScale;
			_subSetData.metallic = _pMaterial->metallic;
			_subSetData.roughness = _pMaterial->roughness;

			// -----------------------------------------------------
			// 描画アイテムの登録
			// -----------------------------------------------------
			Engine::Graphics::LightWeightDrawItem _item = {};
			_item.sortKey.bits.meshID = _cmd.meshRawID;
			_item.sortKey.bits.materialID = _cmd.materialRawID;
			_item.isAnimation = true;
			_item.subIndex = _cmd.subIdx;
			_item.instnaceIndex = SetInstanceData(_instanceData);
			_item.subsetIndex = SetSubSetData(_subSetData);

			switch (_cmd.alphaMode)
			{
			case Engine::Resource::Alpha::Opaque:
				_item.sortKey.bits.psoID = _zpreStatic;
				_item.sortKey.bits.passIndex = _zpreIdx;
				AddItem(_item);
				_item.sortKey.bits.psoID = _gbuffStatic;
				_item.sortKey.bits.passIndex = _opeqIdx;
				AddItem(_item);
				break;
			case Engine::Resource::Alpha::Mask:
				_item.sortKey.bits.psoID = _zpreStatic;
				_item.sortKey.bits.passIndex = _zpreIdx;
				AddItem(_item);
				_item.sortKey.bits.psoID = _gbuffStatic;
				_item.sortKey.bits.passIndex = _opeqIdx;
				AddItem(_item);
				break;
			case Engine::Resource::Alpha::Blend:
				break;
			default:
				break;
			}
		}
	}
	UINT GraphicsEngine::SetInstanceData(const InstanceData& a_instanceData)
	{
		UINT _index = static_cast<UINT>(m_instanceDataVec.size());
		m_instanceDataVec.push_back(a_instanceData);
		return _index;
	}
	UINT GraphicsEngine::SetInstanceData(const MeshInstanceData& a_instanceData)
	{
		UINT _index = static_cast<UINT>(m_meshInstanceDataVec.size());
		m_meshInstanceDataVec.push_back(a_instanceData);
		return _index;
	}
	UINT GraphicsEngine::SetSubSetData(const SubSetData& a_subsetData)
	{
		UINT _index = static_cast<UINT>(m_subSetDataVec.size());
		m_subSetDataVec.push_back(a_subsetData);
		return _index;
	}
	UINT GraphicsEngine::SetMeshMaterialData(const MeshMaterial& a_subsetData)
	{
		UINT _index = static_cast<UINT>(m_meshMaterialDataVec.size());
		m_meshMaterialDataVec.size();
		return _index;
	}
	void GraphicsEngine::AddItem(const LightWeightDrawItem& a_item)
	{
		// アイテム配列に追加
		m_lightWeightDrawItemVec.push_back(a_item);
	}
	std::span<const LightWeightDrawItem> GraphicsEngine::GetPassItems(uint8_t a_passIndex)
	{
		// 探したいパスのキーの最小値と最大値を求める
		uint64_t _minKey = static_cast<uint64_t>(a_passIndex) << 56;
		uint64_t _maxKey = _minKey | 0x00FFFFFFFFFFFFFFull; // 下位56ビットをすべて1にする

		// ソート済み配列から開始位置を見つける
		auto _itStart = std::lower_bound(
			m_lightWeightDrawItemVec.begin(),
			m_lightWeightDrawItemVec.end(),
			_minKey,
			[](const LightWeightDrawItem& a_item, uint64_t a_value)
			{
				return a_item.sortKey.value < a_value;
			}
		);

		// ソート済み配列から終了位置を見つける
		auto _itEnd = std::upper_bound(
			_itStart,		// 開始位置から探す
			m_lightWeightDrawItemVec.end(),
			_maxKey,
			[](uint64_t a_value, const LightWeightDrawItem& a_item)
			{
				return a_value < a_item.sortKey.value;
			}
		);

		return std::span<const LightWeightDrawItem>(_itStart, _itEnd);
	}

	void GraphicsEngine::DrawQueue(Graphics::RenderContext* a_pCtx, uint8_t a_passIndex)
	{
		// キャッシュ
		uint16_t _lassMaterialID = 0xFFFF;
		uint16_t _lastMeshID = 0xFFFF;
		uint8_t _lastPSO = 0xFF;

		// 指定タイプの命令キューを取得
		auto _itemVec = GetPassItems(a_passIndex);
		if (_itemVec.empty()) return;

		for (auto& _item : _itemVec)
		{
			uint8_t  _psoID = _item.GetPSOID();
			uint16_t _materialID = _item.GetMaterialID();
			uint16_t _meshID = _item.GetMeshID();
			// ----------------------------------------------------
			// PSOの切り替え
			// ----------------------------------------------------
			if (_psoID != _lastPSO)
			{
				auto* _pPSO = m_pPipelineStateManager->GetPSO(_psoID);
				if (!_pPSO) continue;
				a_pCtx->SetGraphicPSO(_pPSO);

				_lastPSO = _psoID;
			}
			// ----------------------------------------------------
			// マテリアルのバインド
			// ----------------------------------------------------
			if (_materialID != _lassMaterialID)
			{
				a_pCtx->BindMaterialSRV(5, _materialID);
				_lassMaterialID = _materialID;
			}
			// ----------------------------------------------------
			// メッシュのバインド
			// ----------------------------------------------------
			if (_meshID != _lastMeshID)
			{
				a_pCtx->BindMesh(_meshID);
				_lastMeshID = _meshID;
			}

			// バッファインデックスセット
			a_pCtx->BindIndex(
				_item.instnaceIndex,
				_item.subsetIndex,
				1
			);

			// メッシュ描画
			a_pCtx->Draw(_item.GetMeshID(), _item.subIndex);
		}
	}

	void GraphicsEngine::BindPSO(Graphics::RenderContext* a_pCtx, uint8_t a_psoIndex)
	{
		auto* _pPSO = m_pPipelineStateManager->GetPSO(a_psoIndex);
		if (!_pPSO) return;
		a_pCtx->SetGraphicPSO(_pPSO);
	}

	void GraphicsEngine::CreateGPUCameraData()
	{
		// リセット
		m_cbGPUCamera = {};

		// スクリーン座標を取得
		const auto& _config = Engine::MainEngine::Instance().GetEngineConfig();
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

		// ジッターオフセット計算
		float _jitterX = 0.0f;
		float _jitterY = 0.0f;

		// ジッターオンオフ
		if (true)
		{
			// ハルトンシーケンスのテーブル（ピクセル中心地からのオフセット値 -0.5f ～ 0.5f）
			static const float _sHaltonX[16] = {
				0.000000f, -0.250000f,  0.250000f, -0.375000f,
				0.125000f, -0.125000f,  0.375000f, -0.437500f,
				0.062500f, -0.187500f,  0.312500f, -0.312500f,
				0.187500f, -0.062500f,  0.437500f, -0.468750f
			};
			static const float _sHaltonY[16] = {
				0.000000f,  0.166667f, -0.166667f,  0.500000f,
			   -0.500000f, -0.277778f,  0.055556f,  0.388889f,
			   -0.388889f, -0.055556f,  0.277778f,  0.444444f,
			   -0.222222f,  0.111111f, -0.444444f,  0.222222f
			};
			uint32_t _sampleIndex = m_totlaFrameCount % 16;

			// プロジェクション空間（NDC）のサイズに変換 : NDCは幅が２(-1～1)だから2倍
			_jitterX = (_sHaltonX[_sampleIndex] / (float)_winOp.windowWidth) * 2.0f;
			_jitterY = (_sHaltonY[_sampleIndex] / (float)_winOp.windowHegiht) * 2.0f;
		}

		// カメラの行列を一時的に取得
		DXSM::Matrix _viewMat = m_cbCamera.viewMat;
		DXSM::Matrix _projMat = m_cbCamera.projMat;
		DXSM::Matrix _invViewMat = m_cbCamera.viewInvMat;
		DXSM::Matrix _invProjMat = m_cbCamera.projInvMat;

		// モーションベクター用のジッターなしViewProjを計算
		DXSM::Matrix _nonJitteredViewProj = _viewMat * _projMat;
		DXSM::Matrix _nonJitteredInvViewProj = _nonJitteredViewProj.Invert();

		// 描画用のジッターあり投影行列を作成
		DXSM::Matrix _jitteredProjMat = _projMat;
		_jitteredProjMat._31 += _jitterX;
		_jitteredProjMat._32 += _jitterY;

		// 描画用のジッターありViewProjとその逆行列を計算
		DXSM::Matrix _jitteredViewProj = _viewMat * _jitteredProjMat;
		DXSM::Matrix _invJitteredProj = _jitteredProjMat.Invert();
		DXSM::Matrix _invJitteredViewProj = _jitteredViewProj.Invert();

		// GPU転送用バッファへの詰め込み
		m_cbGPUCamera.pos = m_cbCamera.pos;

		// 通常の描画（SV_Positionの計算）にはジッターありを使う
		m_cbGPUCamera.viewMat			= _viewMat.Transpose();
		m_cbGPUCamera.projMat			= _jitteredProjMat.Transpose();
		m_cbGPUCamera.viewInvMat		= _invViewMat.Transpose();
		m_cbGPUCamera.projInvMat		= _invJitteredProj.Transpose();
		m_cbGPUCamera.viewProjMat		= _jitteredViewProj.Transpose();
		m_cbGPUCamera.invViewProjMat	= _invJitteredViewProj.Transpose();

		// モーションベクターの計算にはジッターなしを使う
		m_cbGPUCamera.nonJitteredProj = _projMat.Transpose();
		m_cbGPUCamera.nonJitteredViewProj = _nonJitteredViewProj.Transpose();
		m_cbGPUCamera.nonJitteredInvViewProj = _nonJitteredInvViewProj.Transpose();

		// 過去フレームのジッターなし行列の処理
		m_cbGPUCamera.prevView = m_prevViewMat.Transpose();
		m_cbGPUCamera.prevProj = m_prevProjMat.Transpose();
		m_cbGPUCamera.prevViewProj = m_prevNonJitteredViewProj.Transpose();

		// 次のフレームのためにジッターなしデータを保存
		m_prevViewMat = _viewMat;
		m_prevProjMat = _projMat;
		m_prevNonJitteredViewProj = _nonJitteredViewProj;

		// フレームカウントを進める
		m_totlaFrameCount++;
	}
}