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
// 1/2・1/4・1/8・1/16 まで縮小しながらボカし、そこから元のサイズへ戻した4枚を
// 平均して1枚のブルーム(BloomColor)にまとめる。
//
// ・縮小率が違うぶんボケの広がりも4段階になっていて、それを重ねることで
//   「芯は明るく、外へ行くほどゆるく広がる」ブルームらしい減衰ができる。
//   同じ広がりを1回の大きなブラーで出そうとするとタップ数が跳ね上がる。
// ・4枚とも入力の時点でフル解像度まで拡大済みなので、シェーダーは同じ座標を
//   そのまま読むだけでよい。
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
			.Add("BloomBlurUp0")	// 1/2  から拡大
			.Add("BloomBlurUp1")	// 1/4  から拡大
			.Add("BloomBlurUp2")	// 1/8  から拡大
			.Add("BloomBlurUp3");	// 1/16 から拡大

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
