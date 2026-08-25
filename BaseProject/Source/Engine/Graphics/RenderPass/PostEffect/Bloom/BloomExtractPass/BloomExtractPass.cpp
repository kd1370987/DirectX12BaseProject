#include "BloomExtractPass.h"

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
// BloomExtractPass
//
// 川瀬式ブルームの入口。ディファードライティングの結果(AfterLighting)から、
// しきい値を超えた高輝度成分だけを抜き出して BloomExtract へ書き出す。
//
// ・入力をトーンマップ前のHDR(AfterLighting)にしているのは、
//   トーンマップ後だと明るさが 0～1 に潰れていて「どれだけ明るいか」が失われるため。
//   出力も同じ R16F にしてレンジを保つ。
// ・TAA より前の絵なのでジッターぶんのちらつきは乗るが、この後 1/16 まで縮小しながら
//   ぼかすので、ほとんど残らない。
// ・無効時はシェーダー側が黒で埋めるので、後段がそのまま走っても絵は変わらない。
//==============================================================================
namespace Engine::Graphics
{
	void AddBloomExtractPass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase
	)
	{
		RenderPassNode _node = {};
		_node.name = "BloomExtractPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Source/PostProcess/Bloom/BloomExtractShader.cso",
			"BloomExtractShader",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=ブルーム設定CB / 1=SRVテーブル(t0) / 2=UAV
		_cpBuilder.SrvTable(1).Add("AfterLighting");

		// HDR : 抽出元と同じ R16F で受ける(高輝度を切り捨てないため)
		// 全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(2, "BloomExtract", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::DontCare, StoreOp::Store);

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
	}
}
