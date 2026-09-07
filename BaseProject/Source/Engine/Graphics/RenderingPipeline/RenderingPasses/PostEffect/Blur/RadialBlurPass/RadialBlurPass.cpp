#include "RadialBlurPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void RadialBlurPass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		// 全画素を書き潰すのでクリアは不要。
		// トーンマップ前なので、メインカラーと同じHDRで受ける
		DeclareOutput("Result", "RadialBlurColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void RadialBlurPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Blur/RadialBlurCS.cso", "RadialBlurCS");
	}

	void RadialBlurPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		//----------------------------------------------------------------------------------
		// 調整値の出どころ
		//
		// カメラが RadialBlurComponent を持っていれば、CamSetShaderSystem が
		// 毎フレーム値を送ってくる(速度に応じて動くのはこちら)。
		// 送られてこないフレームは、このパスがアセットに保存している自分の値を使う。
		//
		// カメラ側を優先しないと、演出でボケや流れが動かなくなる
		//----------------------------------------------------------------------------------
		const RadialBlurOptionCB& _cb = a_context.pGraphicsEngine && a_context.pGraphicsEngine->IsRadialBlurOverride()
			? a_context.pGraphicsEngine->GetRadialBlurData()
			: m_cb;

		// ヒープ・ルートシグネチャ・PSO・SRV/UAV はグラフが張り終えている
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, _cb);
		DispatchFullScreen(a_context);
	}



	void RadialBlurPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("blurCenterX", m_cb.blurCenter.x);
		a_arch.Field("blurCenterY", m_cb.blurCenter.y);
		a_arch.Field("strength", m_cb.strength);
		a_arch.Field("sampleCount", m_cb.sampleCount);
		a_arch.Field("radius", m_cb.radius);
		a_arch.Field("falloff", m_cb.falloff);
		a_arch.Field("enable", m_cb.enable);
	}
}
