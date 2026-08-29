#include "FishEyePass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================
// FishEyePass
//
// メインカラー(AfterTAAColor)を、設定した中心から外へ向かって引き伸ばす。
// 魚眼レンズ越しに覗いたような歪みを出す画面効果。
//
// ・調整値はカメラの持ち物(FishEyeComponent)。CamSetShaderSystem が
//   GraphicsEngine へ毎フレーム詰めたものをここで読む。
// ・書き込み先は別テクスチャ(FishEyeColor)にしてから AfterTAAColor へコピーし直す。
//   同じテクスチャを SRV と UAV で同時に触れないのと、UI/デバッグ線/最終提示が
//   AfterTAAColor という固定名を見ているのを崩さないため(TAA・DoF・ブルーム・
//   ラジアルブラーと同じ作り)。
// ・ラジアルブラーの後に登録すること。引きずった跡ごとレンズで歪んでほしいので、
//   逆にすると歪ませた絵の上をまっすぐ流すことになって噛み合わない。
// ・UI はこれより後のフェーズ(UI)で AfterTAAColor へ直接描かれるので歪まない。
//   UI ごと歪ませたい(湾曲させたい)ものは、UI側の湾曲パラメータで曲げること。
// ・無効時はシェーダー側がそのまま素通しするので、絵は変わらない。
//==============================================================================
namespace Engine::Graphics
{
	void AddFishEyePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// 後続(最終提示)が見ている固定名
		const std::string _mainColor    = "AfterTAAColor";
		const std::string _fishEyeColor = "FishEyeColor";

		// ------------------------------------------------------------------
		// 歪ませるパス
		// ------------------------------------------------------------------
		RenderPassNode _node = {};
		_node.name = "FishEyePass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Source/PostProcess/Distortion/FishEyeCS.cso",
			"FishEyeCS",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=魚眼レンズ設定CB / 1=SRVテーブル(t0) / 2=UAV
		_cpBuilder.SrvTable(1)
			.Add(_mainColor);	// メインカラー

		// HDR : メインカラーと同じ R16F で受ける(トーンマップ前なので潰さない)
		// 全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(2, _fishEyeColor, DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::DontCare, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// 魚眼レンズ調整値(アクティブカメラの FishEyeComponent から積まれたもの)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, a_pGE->GetFishEyeData());

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
		_copyNode.name = "FishEyeCopyPass";
		_copyNode.phase = a_phase;
		RGGlobalsPassBuilder _copyBuilder(&_copyNode);

		_copyBuilder.Copy(_fishEyeColor, _mainColor, DXGI_FORMAT_R16G16B16A16_FLOAT);

		a_pRegistry->RegisterPass(_copyNode);
	}
}
