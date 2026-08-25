#include "BloomCompositePass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"

#include "../BloomCommon.h"

//==============================================================================
// BloomCompositePass
//
// KawaseBlurPass がまとめたブルームを、メインカラー(AfterTAAColor)へ加算合成する。
//
// ・トーンマップ前のHDR段階で足す。トーンマップ後に足すと 1.0 でクランプされて
//   のっぺりした白い板になるが、前で足せばトーンマップが白飛び側へなめらかに収めてくれる。
// ・書き込み先は別テクスチャ(BloomCompositeColor)にしてから AfterTAAColor へコピーし直す。
//   同じテクスチャを SRV と UAV で同時に触れないのと、UI/デバッグ線/最終提示が
//   AfterTAAColor という固定名を見ているのを崩さないため(TAA・DoFパスと同じ作り)。
// ・DoF の後に登録すること。ボケた絵の上にブルームが乗る形になり、
//   逆にすると光がボケて散らない。
// ・UI はこれより後のフェーズ(UI)で AfterTAAColor へ直接描かれるので光らない。
// ・無効時はシェーダー側がそのまま素通しするので、絵は変わらない。
//==============================================================================
namespace Engine::Graphics
{
	void AddBloomCompositePass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase
	)
	{
		// 後続(UI・最終提示)が見ている固定名
		const std::string _mainColor      = "AfterTAAColor";
		const std::string _compositeColor = "BloomCompositeColor";

		// ------------------------------------------------------------------
		// 合成パス
		// ------------------------------------------------------------------
		RenderPassNode _node = {};
		_node.name = "BloomCompositePass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Source/PostProcess/Bloom/BloomCompositeShader.cso",
			"BloomCompositeShader",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=ブルーム設定CB / 1=SRVテーブル(t0,t1) / 2=UAV
		_cpBuilder.SrvTable(1)
			.Add(_mainColor)		// メインカラー
			.Add("BloomColor");		// KawaseBlurPass の出力

		// HDR : メインカラーと同じ R16F で受ける(トーンマップ前なので潰さない)
		// 全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(2, _compositeColor, DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::DontCare, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// ブルーム調整値(オプション → 定数バッファ)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, MakeBloomOptionCB());

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
		_copyNode.name = "BloomCompositeCopyPass";
		_copyNode.phase = a_phase;
		RGGlobalsPassBuilder _copyBuilder(&_copyNode);

		_copyBuilder.Copy(_compositeColor, _mainColor, DXGI_FORMAT_R16G16B16A16_FLOAT);

		a_pRegistry->RegisterPass(_copyNode);
	}
}
