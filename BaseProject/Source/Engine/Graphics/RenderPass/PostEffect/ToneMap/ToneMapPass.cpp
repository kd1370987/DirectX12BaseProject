#include "ToneMapPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================
// ToneMapPass
//
// HDR のまま組み上げてきた最終カラー(AfterTAAColor)を、表示できる 0〜1 へ
// 落として FinalColor へ書き出す。
//
// ・どの曲線で落とすかはグラフィックス設定(ToneMapOption)から定数バッファで送る。
//   絵作りの好みで切り替えたいものなので、シェーダーを差し替える形にはしていない。
// ・入力は UI とデバッグ線まで乗せ終わった AfterTAAColor。
//   なので Present 帯に置く(UI 帯より後ろでないと、UI がトーンマップを通らない)。
// ・出力の FinalColor が「最終テクスチャ」。バックバッファへ載せるのは
//   このあとの CopyToBackBufferPass の仕事で、ここは提示のことを知らない。
//   分けてあるのは、エディターのシーンビューが読むのもこのテクスチャだから
//   (提示と表示で同じ絵を指したい)。
// ・R8G8B8A8_UNORM で受けるのは、バックバッファと同じフォーマットにしておかないと
//   コピーが通らないため。
//==============================================================================
namespace Engine::Graphics
{
	namespace
	{
		// トーンマップ設定 → 定数バッファ
		// ※ HLSL 側 ToneMapOptionData と並びを合わせること
		struct ToneMapOptionData
		{
			uint32_t	type;
			float		exposure;
			float		whitePoint;
			float		pad;
		};

		ToneMapOptionData MakeToneMapOptionCB()
		{
			const auto& _op = Option::OptionManager::GetInstance().GetToneMapOption();

			ToneMapOptionData _cb = {};
			_cb.type       = static_cast<uint32_t>(_op.type);
			_cb.exposure   = _op.exposure;
			_cb.whitePoint = _op.whitePoint;

			return _cb;
		}
	}

	void AddToneMapPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		RenderPassNode _node = {};
		_node.name = "ToneMapPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Source/PostProcess/ToneMap/ToneMapShader.cso",
			"ToneMapShader",
			_csIndex
		);
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetPassPSO(_csIndex);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=トーンマップ設定CB / 1=SRVテーブル(t0) / 2=UAV(u0)
		_cpBuilder.SrvTable(1).Add("AfterTAAColor");

		// 全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(2, "FinalColor", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::DontCare, StoreOp::Store);

		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
				auto* _pCmd = a_pCtx->GetCurrentCmdList();

				// トーンマップ設定(オプション → 定数バッファ)
				a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, MakeToneMapOptionCB());

				// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
				a_pCtx->Dispatch(
					(_winOp.windowWidth + 7) / 8,
					(_winOp.windowHeight + 7) / 8,
					1
				);
			};

		a_pRegistry->RegisterPass(_node);
	}
}
