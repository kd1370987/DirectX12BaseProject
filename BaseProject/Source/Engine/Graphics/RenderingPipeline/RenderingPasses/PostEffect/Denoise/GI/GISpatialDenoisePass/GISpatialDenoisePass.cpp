#include "GISpatialDenoisePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void GISpatialDenoisePass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=調整値CB / 2=SRVテーブル / 3=UAV
		DeclareInput("GI", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 2);

		// GIはハーフ解像度。GBuffer(フル)とは倍率が違う
		DeclareOutput("Result", m_params.resourceName, DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 3);

		ApplyResourceName();
	}

	void GISpatialDenoisePass::ApplyResourceName()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_params.resourceName;

		// GIはハーフ解像度で回る
		_pOut->width = 0;
		_pOut->height = 0;
		_pOut->scale = 0.5f;
	}

	void GISpatialDenoisePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/Denoise/GI/GISpatialDenoiseShader.cso", "GISpatialDenoisePass");
	}

	void GISpatialDenoisePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		// カメラCB(b0) : シェーダー側でワールド座標を復元してエッジ判定に使う
		a_context.pRenderContext->ComputeBindRootCBV(0, a_context.pGraphicsEngine->GetCameraData());
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, m_params.cb);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}



	void GISpatialDenoisePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_params.resourceName);
		a_arch.Field("stepSize", m_params.cb.stepSize);
		a_arch.Field("phiDepth", m_params.cb.phiDepth);
		a_arch.Field("phiNormal", m_params.cb.phiNormal);
		a_arch.Field("phiColor", m_params.cb.phiColor);

		if (a_arch.IsLoading()) ApplyResourceName();
	}
}
