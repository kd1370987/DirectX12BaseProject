#include "Engine/Graphics/RenderingPipeline/RenderingPasses/Lighting/Shadow/RaytracingVolumeShadowPass/RaytracingVolumeShadowPass.h"

#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"

#include "Engine/Graphics/Raytracing/RayEngine.h"

namespace Engine::Graphics::Pipeline
{
	void RaytracingVolumeShadowPass::SetupSlots()
	{
		// シーンの深度 : 視線の終点
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, ROOT_INPUT_SRV);

		// フォグ用の影 : r = 視線に沿った日なたの割合 / g = 歩いた範囲の終わり(m)。
		// 全画素を書き潰すのでクリアは不要
		DeclareOutput("VolumeShadow", "RayVolumeShadow", DXGI_FORMAT_R16G16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, ROOT_OUTPUT_UAV);

		ApplyResolution();
	}

	void RaytracingVolumeShadowPass::ApplyResolution()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("VolumeShadow"));
		if (!_pOut) return;

		const uint32_t _divisor = std::clamp(m_params.resolutionDivisor, 1u, 8u);

		// 描画解像度 × scale : 解像度違いのカメラやリサイズに追従させるため絶対値にはしない
		_pOut->width = 0;
		_pOut->height = 0;
		_pOut->scale = 1.0f / static_cast<float>(_divisor);
	}

	void RaytracingVolumeShadowPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Raytracing/Shadow/RayVolumeShadowCS.cso", "RayVolumeShadowCS");
	}

	void RaytracingVolumeShadowPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList || !a_context.pGraph) return;

		auto* _pCmdList = a_context.pCmdList;
		const LightManager* _pLightManager = _pGE->RefLightManager();
		const SceneView* _pSceneView = _pGE->GetSceneView();

		//----------------------------------------------------------------------------------
		// レイを飛ばさないフレーム
		//
		//   ・影をシャドウマップで求める : フォグはシャドウマップの影を引く
		//   ・歩数 0                     : 作らない設定
		//   ・フォグもダストも濃さ 0     : 平行光の影を使う人が居ない
		//   ・レイトレの世界が無い
		//
		// UAV の出力はグラフがクリアしないので、埋めないと前の使い手の中身が残る
		//----------------------------------------------------------------------------------
		const bool _isShadowMap = (_pLightManager->GetShadowMode() == EDirectionalShadowMode::ShadowMap);
		const bool _isFogUnused =
			(_pSceneView->GetSceneFogData().density <= 0.0f) &&
			(_pSceneView->GetGroundDustData().density <= 0.0f);

		if (_isShadowMap || m_params.stepCount == 0 || _isFogUnused || !a_context.pRayEngine)
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

		// カメラ : 深度から視線を戻す
		_pCtx->ComputeBindRootCBV(ROOT_CAMERA_CB, _pSceneView->GetCameraData());

		// 主光源 : 平行光の向き
		const auto _sunCB = _pLightManager->GetSunLightCB();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmdList, ROOT_SUN_LIGHT_CB, _sunCB);

		// 歩く範囲と歩数
		VolumeShadowCB _cb = {};
		_cb.distance = _pLightManager->GetShadowSettings().distance;
		_cb.frame = m_frameCount++;
		_cb.stepCount = m_params.stepCount;
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmdList, ROOT_PARAM_CB, _cb);

		// TLAS : ルートSRV
		_rayEngine.BindTLAS(_pCtx, ROOT_TLAS);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("VolumeShadow"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}

	void RaytracingVolumeShadowPass::ClearOutput(const PassContext& a_context)
	{
		if (!a_context.pRenderContext) return;

		const Slot* _pOut = FindOutputSlot(MakeSlotID("VolumeShadow"));
		D3D12::GPUResource* _pOutRes = _pOut ? a_context.GetResource(*_pOut) : nullptr;
		if (!_pOutRes) return;

		// r = 日なたの割合 1 / g = 範囲 0(フォグは平行光を遮らずに照らす)
		const float _lit[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
		a_context.pRenderContext->ClearUAV(_pOutRes->GetUAV(), _pOutRes->GetResource(), _lit);
	}

	void RaytracingVolumeShadowPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("stepCount", m_params.stepCount);
		a_arch.Field("resolutionDivisor", m_params.resolutionDivisor);

		if (a_arch.IsLoading()) ApplyResolution();
	}
}
