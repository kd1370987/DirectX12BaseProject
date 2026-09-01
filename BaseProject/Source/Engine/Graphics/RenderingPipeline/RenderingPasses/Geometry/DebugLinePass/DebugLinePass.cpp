#include "DebugLinePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void DebugLinePass::SetupSlots()
	{
		// 深度は読むだけ(線がモデルに隠れるようにする)。
		//
		// 任意にしてあるのは、このパスだけを置いた構成を作れるようにするため。
		// 必須にすると深度を作るパスが無い構成が検証で落ち、
		// パイプラインごとコンパイルされずに旧経路の絵が出る(繋いでいないUIが映る等)
		DeclareInput("Depth", EAccessType::Depth_Read, EPassSlotType::Texture, false);

		// 描き足す先 : 「前段が描いた絵の上に重ねる」という順序をこの線で表す
		DeclareInput("Color", EAccessType::RTV, EPassSlotType::Texture, false);

		Slot& _color = DeclareOutput("Color", "AfterDebugLineColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::RTV);
		_color.loadOp = ELoadOp::Load;
	}

	// 描き足す先が繋がっていれば、そのリソースへ重ねる
	void DebugLinePass::OnLinksResolved()
	{
		FollowInputToOutput("Color", "Color");
	}

	void DebugLinePass::Compile(const PassContext& a_context)
	{
		// 深度が繋がっていなければ深度テストごと切る。
		// PSO側だけ深度ありのままにすると、グラフがDSVを張らないぶんと食い違って
		// 描画そのものが落とされる(#615 DEPTH_STENCIL_FORMAT_MISMATCH_PIPELINE_STATE)
		const Slot* _pDepth = FindInputSlot(MakeSlotID("Depth"));
		const bool _isDepth = (_pDepth && _pDepth->IsConnected());

		SetupRasterShader(
			a_context,
			"Asset/Shader/Source/Debug/DebugLine/DebugLineVS.cso",
			"Asset/Shader/Source/Debug/DebugLine/DebugLinePS.cso",
			// このVSは SV_VertexID / SV_InstanceID だけで頂点を作り、頂点バッファを読まない。
			// レイアウトを宣言すると IA が直前のパスの残したバッファを読もうとして警告が出る
			D3D12::Input::gEmptyLayout,
			"DebugLinePSO",
			[_isDepth](D3D12::GraphicsPipelineDesc& a_pso)
			{
				if (_isDepth)
				{
					a_pso.DepthEnable(true);
					a_pso.DepthWriteMask(false);
					a_pso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
				}
				else
				{
					a_pso.DepthEnable(false);
					a_pso.StencilEnable(false);
				}

				a_pso.desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			},
			EPassHeapMode::Default);
	}

	void DebugLinePass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		if (!_pCtx) return;

		_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		_pCtx->BindCamera();
		_pCtx->BindGraphicsDebugLineBuffer(1);

		_pCtx->DrawShape();
	}

	EPassEditResult DebugLinePass::EditUpdate()
	{
		ImGui::TextDisabled("当たり判定やレイのデバッグ線を描きます");
		return EPassEditResult::None;
	}

	void DebugLinePass::EditNode()
	{}

	void DebugLinePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
