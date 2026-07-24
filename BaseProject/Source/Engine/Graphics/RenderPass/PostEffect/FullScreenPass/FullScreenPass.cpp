#include "FullScreenPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Graphics
{
	void AddFullScreenPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		RenderPassNode _node = {};
		_node.name = "FullScreenPass";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// ======================================================================
		// 依存関係とバインドの宣言
		// ======================================================================
		// TAAパス（またはその後続パス）の最終出力を読み込む
		_rpBuilder.BindSRV(0, "AfterTAAColor");
		_rpBuilder.SetHeapMode(ERGHeapMode::Default);

		uint8_t _staticIndex = RenderPassNode::kInvalidPSOIndex;
		auto& _sPso = _rpBuilder.CreatePSODesc("FullScreenPass", _staticIndex);

		// このVSは SV_VertexID だけで三角形を生成し、頂点バッファを読まない。
		// StaticLayout を宣言すると IA が頂点バッファを要求し、直前のパスが残したバッファを
		// 読もうとして #210(頂点バッファが小さすぎる)警告が出るため、空レイアウトを使う。
		auto* _pBlob = _rpBuilder.SetVS(_sPso, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso", D3D12::Input::gEmptyLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingPS.cso");

		_rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		// デプスとステンシルは無効化（フルスクリーン描画のため）
		_sPso.DepthEnable(false);
		_sPso.StencilEnable(false);

		// ★バックバッファはレンダーグラフにRTVとして要求しないため、ここで手動フォーマット指定
		_sPso.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// PSOが確定したのでグラフに自動セットさせる
		_rpBuilder.SetPassPSO(_staticIndex);

		// ======================================================================
		// 実行関数 : バックバッファへの切り替えと描画のみ
		// ======================================================================
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				// （※ChangeBackBufferの中で、バックバッファへのバリア遷移とOMSetRenderTargetsが行われる想定）
				a_pCtx->ChangeBackBuffer();

				// 画面全体に描画
				a_pCtx->DrawQuad();
			};

		a_pRegistry->RegisterPass(_node);
	}
}
