#include "FinalOutputPass.h"

#include "../../GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void FinalOutputPass::SetupSlots()
	{
		// 受け取った絵をそのまま写すだけなので、読み書きともコピーのアクセスにする。
		// シェーダーを通さないぶん、フォーマットと大きさは入口と出口で揃っている必要がある
		DeclareInput(kInputName, EAccessType::CopySrc);

		// 出力の名前は GraphicsEngine がカメラの最終出力を差し込むときの名前と同じ。
		// 同じ名前なので、仮想リソースの構築でその外部リソースへ合流する
		DeclareOutput(
			"Output",
			GraphicsEngine::kCameraOutputName,
			DXGI_FORMAT_UNKNOWN,		// 実体は外から差し込まれるので、こちらでは決めない
			EAccessType::CopyDst);
	}

	void FinalOutputPass::Compile(const PassContext& a_context)
	{
		(void)a_context;
	}

	void FinalOutputPass::Update(const PassContext& a_context)
	{
		if (!a_context.pCmdList) return;

		// 入口が繋がっていなければ何も写さない。
		// (必須入力なので、本来は Validation の時点で弾かれている)
		const Slot* _pInSlot = FindInputSlot(MakeSlotID(kInputName));
		if (!_pInSlot || !_pInSlot->IsConnected()) return;

		D3D12::GPUResource* _pSrc = a_context.GetResource(*_pInSlot);
		if (!_pSrc) return;

		const Slot* _pOutSlot = FindOutputSlot(MakeSlotID("Output"));
		if (!_pOutSlot) return;

		D3D12::GPUResource* _pDst = a_context.GetResource(*_pOutSlot);
		if (!_pDst) return;

		// ステートはグラフが CopySrc / CopyDst へ遷移済み
		a_context.pCmdList->CopyResource(_pDst->GetResource(), _pSrc->GetResource());
	}

	EPassEditResult FinalOutputPass::EditUpdate()
	{
		ImGui::TextDisabled("このノードの絵がカメラの最終出力になります");

		const Slot* _pInSlot = FindInputSlot(MakeSlotID(kInputName));
		if (_pInSlot && _pInSlot->IsConnected())	ImGui::Text("Input : %s", _pInSlot->name.c_str());
		else										ImGui::TextDisabled("Input : (not connected)");

		// 触れる設定を持たない
		return EPassEditResult::None;
	}

	void FinalOutputPass::EditNode()
	{}

	void FinalOutputPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
