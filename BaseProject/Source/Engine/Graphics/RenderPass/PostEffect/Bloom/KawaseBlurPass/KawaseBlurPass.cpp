#include "KawaseBlurPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================
// KawaseBlurPass
//
// 川瀬式ブルームの合流点。
// 1/2・1/4・1/8・1/16 まで縮小しながらボカした4枚を、平均して1枚のブルーム
// (BloomColor)にまとめる。
//
// ・縮小率が違うぶんボケの広がりも4段階になっていて、それを重ねることで
//   「芯は明るく、外へ行くほどゆるく広がる」ブルームらしい減衰ができる。
//   同じ広がりを1回の大きなブラーで出そうとするとタップ数が跳ね上がる。
// ・4枚は解像度がバラバラのまま渡す。シェーダーがUVでサンプリングするので、
//   拡大はサンプラーのバイリニアが兼ねてくれる。
//   元のサイズへ戻す専用パスを挟むと、パス4本とフル解像度のR16Fが4枚増えるだけで、
//   入力がすでにボケている以上、絵の差はほとんど出ない。
// ・強さは合成パスの intensity で掛けるので、ここでは平均に留めて総量を増やさない。
//==============================================================================
namespace Engine::Graphics
{
	void AddKawaseBlurPass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase
	)
	{
		RenderPassNode _node = {};
		_node.name = "KawaseBlurPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Compute/Bloom/KawaseBlurShader/KawaseBlurShader.cso",
			"KawaseBlurShader",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=SRVテーブル(t0～t3) / 1=UAV
		// 宣言順がそのままシェーダのレジスタ順になるので、縮小率の小さい順に並べる
		_cpBuilder.SrvTable(0)
			.Add("BloomBlurDown0")	// 1/2
			.Add("BloomBlurDown1")	// 1/4
			.Add("BloomBlurDown2")	// 1/8
			.Add("BloomBlurDown3");	// 1/16

		// HDR : ここもトーンマップ前なので R16F のまま保つ
		// 全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(1, "BloomColor", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::DontCare, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 可変値が無いのでディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

			// 実行
			// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
			a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHeight + 7) / 8, 1);
		};

		a_pRegistry->RegisterPass(_node);
	}
}
