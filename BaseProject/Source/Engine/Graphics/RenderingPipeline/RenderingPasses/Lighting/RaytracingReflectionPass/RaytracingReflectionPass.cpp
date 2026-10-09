#include "Engine/Graphics/RenderingPipeline/RenderingPasses/Lighting/RaytracingReflectionPass/RaytracingReflectionPass.h"

#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Graphics/Frame/MeshBufferAllocator/MeshBufferAllocator.h"
#include "Engine/Graphics/Frame/SceneView/AmbientData.h"
#include "Engine/Graphics/Frame/SceneView/SkyData.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Engine/Graphics/Raytracing/RayEngine.h"

namespace Engine::Graphics::Pipeline
{
	void RaytracingReflectionPass::SetupSlots()
	{
		// 反射させる面 : 宣言順がそのままルート定数の並び(深度 → 法線)
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, ROOT_INPUT_SRV);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, ROOT_INPUT_SRV);

		// 反射 : rgb = 反射先の放射輝度 / a = 物に当たったか。
		// 大きさは描画解像度に依らず固定。全画素を書き潰すのでクリアは不要
		DeclareOutput("Reflection", "RayReflection", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, ROOT_OUTPUT_UAV,
			OUTPUT_WIDTH, OUTPUT_HEIGHT);
	}

	void RaytracingReflectionPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Raytracing/Reflection/RayReflectionCS.cso", "RayReflectionCS");
	}

	void RaytracingReflectionPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList || !a_context.pGraph) return;

		auto* _pCmdList = a_context.pCmdList;
		const SceneView* _pSceneView = _pGE->GetSceneView();

		// UAV の出力はグラフがクリアしないので、飛ばさないフレームも埋めないと前の使い手の中身が残る
		auto* _pMA = _pGE->RefMeshBufferAllocator();
		if (!a_context.pRayEngine || !_pMA)
		{
			ClearOutput(a_context);
			return;
		}

		auto& _rayEngine = *a_context.pRayEngine;

		// レイワールド更新(同じフレームで先に誰かが済ませていれば何もしない)
		_rayEngine.Commit(_pCmdList, _pGE->RefRenderDevice()->GetCurrentFrameIndex());
		if (_rayEngine.GetInstanceVec().empty())
		{
			ClearOutput(a_context);
			return;
		}

		// カメラ : 深度から面を戻す
		_pCtx->ComputeBindRootCBV(ROOT_CAMERA_CB, _pSceneView->GetCameraData());

		// 主光源 : 当たった先の直接光
		const auto _sunCB = _pGE->RefLightManager()->GetSunLightCB();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmdList, ROOT_SUN_LIGHT_CB, _sunCB);

		// 空と環境光 : 空に抜けたレイの色と、当たった先の間接光の代わり
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<SkyData>(_pCmdList, ROOT_SKY_CB, _pSceneView->GetSkyData());
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(_pCmdList, ROOT_AMBIENT_CB, _pSceneView->GetAmbientData());

		// 反射の設定
		ReflectionCB _cb = {};
		_cb.maxDistance = m_params.maxDistance;
		_cb.isShadow = m_params.isShadow ? 1u : 0u;
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmdList, ROOT_PARAM_CB, _cb);

		//----------------------------------------------------------------------------------
		// 当たった先を引くためのバッファ
		//
		// グラフのリソースではないので、スロットには乗らない。番号をここで直接渡す。
		// シェーダーは受け取った順に読むので、並びを崩さないこと
		//----------------------------------------------------------------------------------
		// スカイテクスチャはシーンが差し替えるので、無いフレームは無効値で渡す(シェーダーは環境光を使う)
		UINT _skyTexIndex = 0xFFFFFFFFu;
		if (a_context.pResourceManager)
		{
			const auto* _pSkyTex = a_context.pResourceManager->Get(_pSceneView->GetSkyTexture());
			if (_pSkyTex) _skyTexIndex = _pSkyTex->GetSRV().GetIndex();
		}

		const UINT _bufferIndices[] = {
			_rayEngine.GetInstanceBufferSRV().GetIndex(),
			_rayEngine.GetMaterialBufferSRV().GetIndex(),
			_pMA->GetStaticVertexBuffer().GetSRV().GetIndex(),
			_pMA->GetIndexBuffer().GetSRV().GetIndex(),
			_pMA->GetAnimatedVertexBuffer().GetSRV().GetIndex(),
			_skyTexIndex,
		};
		_pCtx->ComputeBindDescriptorIndices(ROOT_BUFFER_SRV, _bufferIndices);

		// TLAS : ルートSRV
		_rayEngine.BindTLAS(_pCtx, ROOT_TLAS);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Reflection"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}

	void RaytracingReflectionPass::ClearOutput(const PassContext& a_context)
	{
		if (!a_context.pRenderContext) return;

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Reflection"));
		D3D12::GPUResource* _pOutRes = _pOut ? a_context.GetResource(*_pOut) : nullptr;
		if (!_pOutRes) return;

		// 反射なし : 受け取る側は a = 0 を空として扱うので、色も 0 にしておく
		const float _none[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		a_context.pRenderContext->ClearUAV(_pOutRes->GetUAV(), _pOutRes->GetResource(), _none);
	}

	void RaytracingReflectionPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("maxDistance", m_params.maxDistance);
		a_arch.Field("isShadow", m_params.isShadow);
	}
}
