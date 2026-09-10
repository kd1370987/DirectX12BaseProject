#include "TestClearPass.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Graphics::Pipeline
{
	void TestClearPass::SetupSlots()
	{
		// 画面と同じ大きさ・同じフォーマットで作る。
		// FinalOutputPass はコピーで写すので、最終出力と揃っていないと写せない
		DeclareOutput(
			kOutputName,
			"TestColor",
			DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::RTV);
	}

	void TestClearPass::Compile(const PassContext& a_context)
	{
		(void)a_context;
	}

	void TestClearPass::Update(const PassContext& a_context)
	{
		if (!a_context.pCmdList) return;

		const Slot* _pOutSlot = FindOutputSlot(MakeSlotID(kOutputName));
		if (!_pOutSlot) return;

		D3D12::GPUResource* _pResource = a_context.GetResource(*_pOutSlot);
		if (!_pResource) return;

		// ステートはグラフが RENDER_TARGET へ遷移済み。
		// レンダーターゲットの切り替えもグラフが済ませてあるので、ここは塗るだけでよい
		if (!a_context.pHeapManager) return;

		const D3D12_CPU_DESCRIPTOR_HANDLE _rtv =
			a_context.pHeapManager->GetCPU(_pResource->GetRTV());

		const float _color[4] = { m_params.clearColor.r, m_params.clearColor.g, m_params.clearColor.b, m_params.clearColor.a };
		a_context.pCmdList->ClearRenderTargetView(_rtv, _color, 0, nullptr);
	}



	void TestClearPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("clearColor", m_params.clearColor);
	}
}
