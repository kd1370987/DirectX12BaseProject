#include "BlendPass.h"
#include "../../RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void BlendPass::SetupSlots()
	{
		// 入力
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 0);		// 元画像
		DeclareInput("BlendColor", EAccessType::SRV, EPassSlotType::Texture, true, 0);	// 重ねる画像

		// 出力
		DeclareOutput("Color","BlendColor",DXGI_FORMAT_R8G8B8A8_UNORM,EAccessType::UAV,EPassSlotType::Texture,false,1);
	}

	void BlendPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Blend/BlendCS.cso","BlendShader");
	}

	void BlendPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		// ヒープ・ルートシグネチャ・PSO・ディスクリプタテーブルはグラフが張り終えている。
		// ここで回さないとシェーダーが一度も走らず、出力リソースが空のまま次段へ渡る
		DispatchFullScreen(a_context);
	}

	EPassEditResult BlendPass::EditUpdate()
	{
		return EPassEditResult();
	}

	void BlendPass::EditNode()
	{}

	void BlendPass::Archive(Engine::Persistence::Archive& a_arch)
	{}
}