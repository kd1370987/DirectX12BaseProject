#include "FinalOutputPass.h"

#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void FinalOutputPass::SetupSlots()
	{
		// 受け取った絵をそのまま写すだけなので、読み書きともコピーのアクセスにする。
		// シェーダーを通さないぶん、フォーマットと大きさは入口と出口で揃っている必要がある
		DeclareInput(kInputName, EAccessType::CopySrc);

		// 出力先は GraphicsEngine がカメラの最終出力として差し込む外部リソース。
		//
		// リソースの同一性はふつう「作ったパス + 出力ピン」で決まるが、
		// 差し込む側にはパスが居ない。ここだけは名前で待ち合わせる
		DeclareImportedOutput(
			"Output",
			GraphicsEngine::kCameraOutputName,
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

		// CopyResource はフォーマットも大きさも同じでないと成立しない。
		//
		// 違うまま積むとコピーが落とされ、画面が真っ黒になるだけで理由が出ない。
		// (例 : HDRのまま出口へ繋ぐと R16G16B16A16_FLOAT -> R8G8B8A8_UNORM になる)
		// 出口はシェーダーを通さないので、最後はバックバッファと同じ形にして渡すこと
		const D3D12_RESOURCE_DESC _srcDesc = _pSrc->GetResource()->GetDesc();
		const D3D12_RESOURCE_DESC _dstDesc = _pDst->GetResource()->GetDesc();

		if (_srcDesc.Format != _dstDesc.Format ||
			_srcDesc.Width != _dstDesc.Width ||
			_srcDesc.Height != _dstDesc.Height)
		{
			if (!m_isMismatchReported)
			{
				m_isMismatchReported = true;
				ENGINE_WARNING(
					"[FinalOutputPass] 出口へ繋いだ絵が画面と違う形です。トーンマップなどで揃えてください "
					"(入口 fmt=%d %llux%u / 出口 fmt=%d %llux%u)",
					static_cast<int>(_srcDesc.Format), _srcDesc.Width, _srcDesc.Height,
					static_cast<int>(_dstDesc.Format), _dstDesc.Width, _dstDesc.Height);
			}
			return;
		}

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
