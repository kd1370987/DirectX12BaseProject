#include "ShadowSpatialDenoisePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void ShadowSpatialDenoisePass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Shadow", EAccessType::SRV, EPassSlotType::Texture, true, 1);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 1);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		// GBufferと同じフル解像度
		DeclareOutput("Result", m_params.resourceName, DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);

		ApplyResourceName();
	}

	void ShadowSpatialDenoisePass::ApplyResourceName()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_params.resourceName;
	}

	void ShadowSpatialDenoisePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/Denoise/Shadow/ShadowSpatialDenoiseShader.cso", "ShadowSpatialDenoisePass");
	}

	void ShadowSpatialDenoisePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		// このシェーダーはルート直置きのCBを使う
		a_context.pRenderContext->ComputeBindRootCBV(0, m_params.cb);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}



	void ShadowSpatialDenoisePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_params.resourceName);
		a_arch.Field("stepSize", m_params.cb.stepSize);
		a_arch.Field("phiDepth", m_params.cb.phiDepth);
		a_arch.Field("phiNormal", m_params.cb.phiNormal);
		a_arch.Field("phiColor", m_params.cb.phiColor);

		if (a_arch.IsLoading()) ApplyResourceName();
	}
}
