#include "DoFPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "../../../../../Option/OptionManager.h"

//==============================================================================
// DoFPass
//
// CoCPass が作った CoC を見て、メインカラー(AfterTAAColor)をボカす。
//
// ・TAA の後に掛ける。ボカした後に TAA を掛けると履歴がにじんで汚れるため。
// ・書き込み先は別テクスチャ(DoFColor)にしてから AfterTAAColor へコピーし直す。
//   同じテクスチャを SRV と UAV で同時に触れないのと、UI/デバッグ線/最終提示が
//   AfterTAAColor という固定名を見ているのを崩さないため(TAAパスと同じ作り)。
// ・UI は DoF より後のフェーズ(UI)で AfterTAAColor へ直接描かれるのでボケない。
// ・無効時はシェーダー側がそのまま素通しするので、絵は変わらない。
//==============================================================================
namespace Engine::Graphics
{
	void AddDoFPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// 後続(UI・最終提示)が見ている固定名
		const std::string _mainColor = "AfterTAAColor";
		const std::string _dofColor  = "DoFColor";

		// ------------------------------------------------------------------
		// ぼかしパス
		// ------------------------------------------------------------------
		RenderPassNode _node = {};
		_node.name = "DoFPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Compute/PostEffect/DoF/DoFShader.cso",
			"DoFShader",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=DoF設定CB / 1=SRVテーブル(t0,t1) / 2=UAV
		_cpBuilder.SrvTable(1)
			.Add(_mainColor)	// メインカラー
			.Add("CoC");		// CoCPass の出力

		// HDR : メインカラーと同じ R16F で受ける(トーンマップ前なので潰さない)
		_cpBuilder.BindUAV(2, _dofColor, DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::DontCare, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// DoF調整値(アクティブカメラの FocusParamComponent から積まれたもの)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, a_pGE->GetDoFData());

			// 実行
			// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
			a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHeight + 7) / 8, 1);
		};

		a_pRegistry->RegisterPass(_node);

		// ------------------------------------------------------------------
		// コピーパス
		// 結果をメインカラーの固定名へ戻す。宣言だけで完結するので実行関数は不要
		// ------------------------------------------------------------------
		RenderPassNode _copyNode = {};
		_copyNode.name = "DoFCopyPass";
		_copyNode.phase = a_phase;
		RGGlobalsPassBuilder _copyBuilder(&_copyNode);

		_copyBuilder.Copy(_dofColor, _mainColor, DXGI_FORMAT_R16G16B16A16_FLOAT);

		a_pRegistry->RegisterPass(_copyNode);
	}
}
