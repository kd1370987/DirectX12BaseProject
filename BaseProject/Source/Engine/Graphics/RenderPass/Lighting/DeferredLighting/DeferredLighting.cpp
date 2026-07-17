#include "DeferredLighting.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddDeferredLighting(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "DeferredLighting";
		_node.phase = a_phase;
		RGComputePassBuilder _rpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _rpBuilder.SetShader(
			"Asset/Shader/Compute/Lighting/DeferredLighting/DeferredLightingShader.cso",
			"DeferredLightingShader",
			_csIndex
		);
		// ルートシグネチャ
		_rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_rpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言。
		// ここでの宣言順がそのままシェーダのレジスタ順（t0～t6）になる
		_rpBuilder.SrvTable(2)
			.Add("GBufferAlbedo")
			.Add("GBufferNormal")
			.Add("GBufferMaterial")
			.Add("GBufferEmissiv")
			.Add("Depth")
			.Add("AffterDLShadowTempAccumu")
			.Add("FinalFullRay");

		_rpBuilder.BindUAV(3, "AffterLighting", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 静的に宣言しきれない定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// カメラ
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(_pCmd, 0, a_pGE->GetCameraData());

			// アンビエントカラー
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(_pCmd, 1, a_pGE->GetAmbientData());

			// 実行
			a_pCtx->Dispatch(_winOp.windowWidth / 8, _winOp.windowHegiht / 8, 1);
		};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
