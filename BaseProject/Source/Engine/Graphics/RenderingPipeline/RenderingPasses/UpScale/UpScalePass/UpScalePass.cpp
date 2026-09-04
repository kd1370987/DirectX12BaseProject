#include "UpScalePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/Resource/VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	void UpScalePass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=調整値CB / 2〜4=SRV(個別) / 5=UAV
		// このシェーダーはテーブルではなく個別のルートパラメータへ張る
		DeclareInput("GI", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 3);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 4);

		// 出力はフル解像度
		DeclareOutput("Result", "UpScaledGI", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 5);
	}

	void UpScalePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/UpScale/UpScaleCS.cso", "FullRaytracingUpScaleShader");
	}

	void UpScalePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		// カメラCB(b0) : シェーダー側でワールド座標を復元してエッジ判定に使う
		a_context.pRenderContext->ComputeBindRootCBV(0, a_context.pGraphicsEngine->GetCameraData());

		// 倍率は繋がれた解像度から求める。
		// GIがハーフ解像度なら2。ノードの繋ぎ替えで解像度が変わっても付いてくる
		UpScaleCB _cb = m_cb;
		_cb.scaleRatio = CalcScaleRatio(a_context);

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, _cb);

		DispatchFullScreen(a_context);
	}

	// 入力に対する出力の倍率。
	// 繋がれているリソースの実サイズから求めるので、ハーフでもクォーターでも合う
	float UpScalePass::CalcScaleRatio(const PassContext& a_context) const
	{
		if (!a_context.pGraph) return m_cb.scaleRatio;

		const Slot* _pIn = FindInputSlot(MakeSlotID("GI"));
		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pIn || !_pOut) return m_cb.scaleRatio;

		const VirtualResource* _pInRes = a_context.pGraph->GetVirtualResource(_pIn->resourceID);
		const VirtualResource* _pOutRes = a_context.pGraph->GetVirtualResource(_pOut->resourceID);
		if (!_pInRes || !_pOutRes) return m_cb.scaleRatio;

		const float _inWidth = static_cast<float>(_pInRes->GetWidth());
		const float _outWidth = static_cast<float>(_pOutRes->GetWidth());
		if (_inWidth <= 0.0f || _outWidth <= 0.0f) return m_cb.scaleRatio;

		return _outWidth / _inWidth;
	}

	EPassEditResult UpScalePass::EditUpdate()
	{
		bool _isEdit = false;

		ImGui::TextDisabled("ScaleRatio : %.2f (繋がれた解像度から自動)", m_cb.scaleRatio);
		_isEdit |= ImGui::DragFloat("DepthSigma", &m_cb.depthSigma, 0.001f, 0.0f);
		_isEdit |= ImGui::DragFloat("NormalPower", &m_cb.normalPower, 0.1f, 0.0f);

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void UpScalePass::EditNode()
	{}

	void UpScalePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("depthSigma", m_cb.depthSigma);
		a_arch.Field("normalPower", m_cb.normalPower);
	}
}
