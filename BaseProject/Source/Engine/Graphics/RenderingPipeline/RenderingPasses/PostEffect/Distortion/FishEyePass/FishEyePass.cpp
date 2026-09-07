#include "FishEyePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void FishEyePass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		DeclareOutput("Result", "FishEyeColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void FishEyePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Distortion/FishEyeCS.cso", "FishEyeCS");
	}

	void FishEyePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		//----------------------------------------------------------------------------------
		// 調整値の出どころ
		//
		// カメラが FishEyeComponent を持っていれば、CamSetShaderSystem が
		// 毎フレーム値を送ってくる(速度に応じて動くのはこちら)。
		// 送られてこないフレームは、このパスがアセットに保存している自分の値を使う。
		//
		// カメラ側を優先しないと、演出でボケや流れが動かなくなる
		//----------------------------------------------------------------------------------
		const FishEyeOptionCB& _cb = a_context.pGraphicsEngine && a_context.pGraphicsEngine->IsFishEyeOverride()
			? a_context.pGraphicsEngine->GetFishEyeData()
			: m_cb;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, _cb);
		DispatchFullScreen(a_context);
	}



	void FishEyePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("centerX", m_cb.center.x);
		a_arch.Field("centerY", m_cb.center.y);
		a_arch.Field("strength", m_cb.strength);
		a_arch.Field("enable", m_cb.enable);
	}
}
