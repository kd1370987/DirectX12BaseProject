#include "GaussianBlurPass.h"

#include "../../RenderContext/RenderContext.h"
#include "../RenderGraph/RenderGraph.h"

namespace Engine::Graphics::Pipeline
{
	void GaussianBlurPass::SetupSlots()
	{
		// ルートパラメータ : 0=ブラー設定CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		DeclareOutput("Result", m_resourceName, DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);

		ApplyOutputScale();
	}

	void GaussianBlurPass::ApplyOutputScale()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_resourceName;

		// width / height は 0 のままにして、描画解像度 × scale で決めさせる
		_pOut->width = 0;
		_pOut->height = 0;
		_pOut->scale = m_outputScale;
	}

	void GaussianBlurPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Blur/GaussianBlurShader.cso", "GaussianBlurShader");
	}

	void GaussianBlurPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList || !a_context.pGraph) return;

		const Slot* _pIn = FindInputSlot(MakeSlotID("Color"));
		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pIn || !_pOut) return;

		// 1テクセルぶんのUVは入力の解像度から求める。
		// 縮小段では入力と出力で解像度が違うので、出力側で計算すると滲み方がずれる
		const VirtualResource* _pInRes = a_context.pGraph->GetVirtualResource(_pIn->resourceHandle);
		if (!_pInRes) return;

		const float _srcWidth = static_cast<float>(_pInRes->GetWidth());
		const float _srcHeight = static_cast<float>(_pInRes->GetHeight());
		if (_srcWidth <= 0.0f || _srcHeight <= 0.0f) return;

		GaussianBlurCB _cb = {};
		_cb.srcTexelSize = { 1.0f / _srcWidth, 1.0f / _srcHeight };
		_cb.sigma = m_sigma;
		_cb.tapRadius = m_tapRadius;
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, _cb);

		// 回すのは出力の解像度
		DispatchForSlot(a_context, *_pOut);
	}

	EPassEditResult GaussianBlurPass::EditUpdate()
	{
		bool _isParam = false;
		bool _isStructure = false;

		// 出力リソース名 : 縮小段を複数置くときに名前が被らないようにする
		char _nameBuf[128] = {};
		std::snprintf(_nameBuf, sizeof(_nameBuf), "%s", m_resourceName.c_str());
		if (ImGui::InputText("ResourceName", _nameBuf, sizeof(_nameBuf)))
		{
			m_resourceName = _nameBuf;
			_isStructure = true;
		}

		// 解像度が変わるとテクスチャを作り直すので組み直しが要る
		if (ImGui::DragFloat("OutputScale", &m_outputScale, 0.01f, 0.01f, 1.0f)) _isStructure = true;

		_isParam |= ImGui::DragFloat("Sigma", &m_sigma, 0.01f, 0.01f, 16.0f);
		_isParam |= ImGui::DragInt("TapRadius", &m_tapRadius, 1, 1, 8);

		if (_isStructure)
		{
			ApplyOutputScale();
			return EPassEditResult::Structure;
		}
		return _isParam ? EPassEditResult::Param : EPassEditResult::None;
	}

	void GaussianBlurPass::EditNode()
	{}

	void GaussianBlurPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_resourceName);
		a_arch.Field("outputScale", m_outputScale);
		a_arch.Field("sigma", m_sigma);
		a_arch.Field("tapRadius", m_tapRadius);

		// 値が入ったのはスロットを作った後なので、ここで反映し直す
		if (a_arch.IsLoading()) ApplyOutputScale();
	}
}
