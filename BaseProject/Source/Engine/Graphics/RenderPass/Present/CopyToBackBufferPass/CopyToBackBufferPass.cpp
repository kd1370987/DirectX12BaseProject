#include "CopyToBackBufferPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

//==============================================================================
// CopyToBackBufferPass
//
// 最終テクスチャ(FinalColor)をバックバッファへコピーするだけのパス。
// 絵に手は加えない。
//
// ・バックバッファはレンダーグラフの管理外なので、遷移はここで手動で張る。
//   コピーが済んだら元の状態(RENDER_TARGET)へ戻す。
//   戻さないと、このあとエディターの ImGui が同じバックバッファへ描くときに
//   状態が食い違う。
// ・FinalColor 側の COPY_SOURCE への遷移と、トーンマップパスとの実行順は
//   CopySrc の宣言だけでグラフが面倒を見てくれる。
// ・トーンマップと分けてあるのは、シーンビューに出す絵と実際に提示する絵を
//   同じテクスチャにしておきたいため。1つのパスにまとめると
//   「提示のついでに作ったもの」になり、他から参照しづらくなる。
//==============================================================================
namespace Engine::Graphics
{
	void AddCopyToBackBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		RenderPassNode _node = {};
		_node.name = "CopyToBackBufferPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダーもPSOも持たない。コピーするだけ。
		// 読み込みを宣言しておくことで、
		//   ・トーンマップパスより後ろに並ぶ(FinalColor を書く側 → 読む側)
		//   ・COPY_SOURCE への遷移が自動で入る
		// の2つがグラフ側で解決される
		const RGResourceRef _finalRef = _cpBuilder.CopySrc("FinalColor");

		_node.executeFunc = [_finalRef](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				ID3D12Resource* _pFinal = a_res.Resource(_finalRef)->GetResource();
				ID3D12Resource* _pBackBuffer = D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffer();
				if (!_pFinal || !_pBackBuffer) return;

				// バックバッファだけ手動で遷移させる(グラフの管理外のため)
				a_pCtx->Transition(_pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);

				a_pCtx->ResourceCopy(_pFinal, _pBackBuffer);

				// 元へ戻す。このあとエディターが同じバックバッファへ ImGui を描く
				a_pCtx->Transition(_pBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
			};

		a_pRegistry->RegisterPass(_node);
	}
}
