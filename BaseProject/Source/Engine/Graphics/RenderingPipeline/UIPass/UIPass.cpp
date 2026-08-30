#include "UIPass.h"

#include "../../RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void UIPass::SetupSlots()
	{
		// すでに描かれている絵へ重ねるので Load
		Slot& _color = DeclareOutput("Color", "AfterTAAColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::RTV);
		_color.loadOp = ELoadOp::Load;
	}

	void UIPass::Compile(const PassContext& a_context)
	{
		SetupRasterShader(
			a_context,
			"Asset/Shader/Source/UI/UIVS.cso",
			"Asset/Shader/Source/UI/UIPS.cso",
			D3D12::Input::gParticleInputLayout,
			"UIPso",
			[](D3D12::GraphicsPipelineDesc& a_pso)
			{
				// UIは深度を使わない。DSVを張らないので深度フォーマットもUNKNOWNのまま。
				// (でないと #615 DEPTH_STENCIL_FORMAT_MISMATCH_PIPELINE_STATE になる)
				a_pso.DepthEnable(false);
				a_pso.StencilEnable(false);

				// UIクアッドの巻き順は表裏が混在しており、裏面カリングだと三角形が消える。
				// スクリーン向きの板ポリなのでカリング自体を無効化する
				a_pso.CullMode(D3D12_CULL_MODE_NONE);

				// 半透明UIをシーンへアルファ合成する
				a_pso.BlendEnable(true);
				a_pso.SrcBlend(D3D12_BLEND_SRC_ALPHA);
				a_pso.DestBlend(D3D12_BLEND_INV_SRC_ALPHA);
				a_pso.BlendOp(D3D12_BLEND_OP_ADD);
				a_pso.SrcBlendAlpha(D3D12_BLEND_ONE);
				a_pso.DestBlendAlpha(D3D12_BLEND_INV_SRC_ALPHA);
				a_pso.BlendOpAlpha(D3D12_BLEND_OP_ADD);
			},
			EPassHeapMode::BindlessWithSampler);
	}

	void UIPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		if (!_pCtx) return;

		// ヒープ・ルートシグネチャ・PSO・レンダーターゲットはグラフが張り終えている。
		// DrawPolygonInstancing はトポロジを設定しないので、ここで明示する
		_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// UIインスタンスバッファ(t0)を張るのは DrawUI の中。
		// 湾曲するUIは板ポリが変わるぶんドローが分かれ、その都度張り直す必要がある
		_pCtx->DrawUI(0);
	}

	EPassEditResult UIPass::EditUpdate()
	{
		ImGui::TextDisabled("深度を持たないので、積んだ順がそのまま前後になります");
		return EPassEditResult::None;
	}

	void UIPass::EditNode()
	{}

	void UIPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
