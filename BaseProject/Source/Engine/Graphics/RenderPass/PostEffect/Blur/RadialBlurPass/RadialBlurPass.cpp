#include "RadialBlurPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================
// RadialBlurPass
//
// メインカラー(AfterTAAColor)を、設定した中心から放射状に引きずってボカす。
// 加速時のスピード感や被弾時の衝撃を出す画面効果。
//
// ・調整値はカメラの持ち物(RadialBlurComponent)。CamSetShaderSystem が
//   GraphicsEngine へ毎フレーム詰めたものをここで読む。
// ・書き込み先は別テクスチャ(RadialBlurColor)にしてから AfterTAAColor へコピーし直す。
//   同じテクスチャを SRV と UAV で同時に触れないのと、UI/デバッグ線/最終提示が
//   AfterTAAColor という固定名を見ているのを崩さないため(TAA・DoF・ブルームと同じ作り)。
// ・ブルーム合成の後に登録すること。光ったところごと流れてほしいので、
//   逆にすると引きずった跡だけが後から光ってしまう。
// ・UI はこれより後のフェーズ(UI)で AfterTAAColor へ直接描かれるので流れない。
// ・無効時はシェーダー側がそのまま素通しするので、絵は変わらない。
//==============================================================================
namespace Engine::Graphics
{
	void AddRadialBlurPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// 後続(UI・最終提示)が見ている固定名
		const std::string _mainColor   = "AfterTAAColor";
		const std::string _radialColor = "RadialBlurColor";

		// ------------------------------------------------------------------
		// ぼかしパス
		// ------------------------------------------------------------------
		RenderPassNode _node = {};
		_node.name = "RadialBlurPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Source/PostProcess/Blur/RadialBlurCS.cso",
			"RadialBlurCS",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=ラジアルブラー設定CB / 1=SRVテーブル(t0) / 2=UAV
		_cpBuilder.SrvTable(1)
			.Add(_mainColor);	// メインカラー

		// HDR : メインカラーと同じ R16F で受ける(トーンマップ前なので潰さない)
		// 全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(2, _radialColor, DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::DontCare, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// ラジアルブラー調整値(アクティブカメラの RadialBlurComponent から積まれたもの)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, a_pGE->GetRadialBlurData());

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
		_copyNode.name = "RadialBlurCopyPass";
		_copyNode.phase = a_phase;
		RGGlobalsPassBuilder _copyBuilder(&_copyNode);

		_copyBuilder.Copy(_radialColor, _mainColor, DXGI_FORMAT_R16G16B16A16_FLOAT);

		a_pRegistry->RegisterPass(_copyNode);
	}
}
