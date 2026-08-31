#include "CopyPass.h"

namespace Engine::Graphics::Pipeline
{
	DXGI_FORMAT CopyPass::ToFormat(int a_index)
	{
		switch (a_index)
		{
		case 1:  return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case 2:  return DXGI_FORMAT_R16G16_FLOAT;
		case 3:  return DXGI_FORMAT_R32_FLOAT;
		case 4:  return DXGI_FORMAT_R16_FLOAT;
		default: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	const char* CopyPass::ToFormatName(int a_index)
	{
		switch (a_index)
		{
		case 1:  return "R16G16B16A16_FLOAT";
		case 2:  return "R16G16_FLOAT";
		case 3:  return "R32_FLOAT";
		case 4:  return "R16_FLOAT";
		default: return "R8G8B8A8_UNORM";
		}
	}

	void CopyPass::SetupSlots()
	{
		// シェーダーを通さないので、読み書きともコピーのアクセスにする
		DeclareInput("Source", EAccessType::CopySrc);

		DeclareOutput("Result", m_resourceName, ToFormat(m_formatIndex), EAccessType::CopyDst,
			EPassSlotType::Texture, m_isTemporal);
	}

	void CopyPass::ApplyOutput()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_resourceName;
		_pOut->format = ToFormat(m_formatIndex);
		_pOut->isTemporal = m_isTemporal;
	}

	void CopyPass::Compile(const PassContext& a_context)
	{
		(void)a_context;
	}

	void CopyPass::Update(const PassContext& a_context)
	{
		if (!a_context.pCmdList) return;

		const Slot* _pIn = FindInputSlot(MakeSlotID("Source"));
		if (!_pIn || !_pIn->IsConnected()) return;

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		D3D12::GPUResource* _pSrc = a_context.GetResource(*_pIn);
		D3D12::GPUResource* _pDst = a_context.GetResource(*_pOut);
		if (!_pSrc || !_pDst) return;

		// ステートはグラフが CopySrc / CopyDst へ遷移済み
		a_context.pCmdList->CopyResource(_pDst->GetResource(), _pSrc->GetResource());
	}

	EPassEditResult CopyPass::EditUpdate()
	{
		bool _isStructure = false;

		char _nameBuf[128] = {};
		std::snprintf(_nameBuf, sizeof(_nameBuf), "%s", m_resourceName.c_str());
		if (ImGui::InputText("ResourceName", _nameBuf, sizeof(_nameBuf)))
		{
			m_resourceName = _nameBuf;
			_isStructure = true;
		}

		if (ImGui::BeginCombo("Format", ToFormatName(m_formatIndex)))
		{
			for (int _i = 0; _i < kFormatCount; ++_i)
			{
				const bool _isSelected = (m_formatIndex == _i);
				if (ImGui::Selectable(ToFormatName(_i), _isSelected))
				{
					m_formatIndex = _i;
					_isStructure = true;
				}
				if (_isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (ImGui::Checkbox("History", &m_isTemporal)) _isStructure = true;
		ImGui::TextDisabled("入力と同じフォーマット・大きさにすること");

		if (!_isStructure) return EPassEditResult::None;

		// リソースを作り直すので組み直しが要る
		ApplyOutput();
		return EPassEditResult::Structure;
	}

	void CopyPass::EditNode()
	{}

	void CopyPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_resourceName);
		a_arch.Field("formatIndex", m_formatIndex);
		a_arch.Field("isTemporal", m_isTemporal);

		// 値が入ったのはスロットを作った後なので、ここで反映し直す
		if (a_arch.IsLoading()) ApplyOutput();
	}
}
