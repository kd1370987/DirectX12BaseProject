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
		// ======================================================================
			// ランタイムデータ作成
			// ======================================================================
		struct RuntimeData
		{
			uint8_t staticIndex;
			D3D12::PipelineStateManager* pPSOManager;
			ID3D12RootSignature* pRootSig;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;

		RenderPassNode _node = {};
		_node.name = "FullScreenPass";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// ======================================================================
		// 依存関係構築（Setupフェーズ）
		// ======================================================================
		// TAAパス（またはその後続パス）の最終出力を読み込む
		_rpBuilder.ReadSRV("AffterTAAColor");

		auto& _sPso = _rpBuilder.CreatePSODesc("FullScreenPass", _spPassData->staticIndex);

		// SetVS には InputLayout の指定が必要なため StaticLayout を渡す
		auto* _pBlob = _rpBuilder.SetVS(_sPso, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingPS.cso");

		_spPassData->pRootSig = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		// デプスとステンシルは無効化（フルスクリーン描画のため）
		_sPso.DepthEnable(false);
		_sPso.StencilEnable(false);

		// ★バックバッファはレンダーグラフにRTVとして要求しないため、ここで手動フォーマット指定
		_sPso.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// ======================================================================
		// 実行関数の登録
		// ======================================================================
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				auto* _pRenderGraph = a_pGE->RefRenderGraph();
				if (!_pRenderGraph) return;

				auto* _pCmd = a_pCtx->GetCurrentCmdList();

				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->staticIndex);
				a_pCtx->SetGraphicPSO(_pPSO);

				// =======================================================
				// 1. SRVの取得とバインド
				// =======================================================
				// レンダーグラフから新しいインターフェースでパス専用のSRVを取得
				auto _mainTex = _pRenderGraph->GetPassSRV(a_passIndex, "AffterTAAColor");
				a_pCtx->BindSRV(0, _mainTex);

				// =======================================================
				// 2. レンダーターゲットの切り替えと描画
				// =======================================================
				// （※ChangeBackBufferの中で、バックバッファへのバリア遷移とOMSetRenderTargetsが行われる想定）
				a_pCtx->ChangeBackBuffer();

				// 画面全体に描画
				a_pCtx->DrawQuad();
			};

		a_pRegistry->RegisterPass(_node);
	}
}