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

		DeclareOutput("Result", m_params.resourceName, ToFormat(m_params.formatIndex), EAccessType::CopyDst,
			EPassSlotType::Texture, m_params.isTemporal);
	}

	void CopyPass::ApplyOutput()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_params.resourceName;
		_pOut->format = ToFormat(m_params.formatIndex);
		_pOut->isTemporal = m_params.isTemporal;
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



	void CopyPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_params.resourceName);
		a_arch.Field("formatIndex", m_params.formatIndex);
		a_arch.Field("isTemporal", m_params.isTemporal);

		// 値が入ったのはスロットを作った後なので、ここで反映し直す
		if (a_arch.IsLoading()) ApplyOutput();
	}
}
