#include "DebugLinePass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../RenderPassRegistry/RenderPassRegistry.h"
namespace Engine::Graphics
{
	void AddDebugLinePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticPsoIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "DebugLinePass";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定


		// 依存関係構築
		_rpBuilder.ReadDepth("Depth");

		_rpBuilder.WriteRTV("AffterTAAColor", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Load, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("DebugLinePSO", _spPassData->staticPsoIndex);
		auto* _pBlob = _rpBuilder.SetVS(_sPso, "Asset/Shader/Source/DebugLineShader/DebugLineVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/DebugLineShader/DebugLinePS.cso");
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		_spPassData->pRootSig = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		_sPso.desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				// ヒープ・ルートシグネチャ・PSOセット
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);
				a_pCtx->SetGraphicPSO(_spPassData->staticPsoIndex);

				a_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

				// カメラのバインド
				a_pCtx->BindGraphicsCamera();

				// デバッグライン描画用構造体バッファバインド
				a_pCtx->BindGraphicsDebugLineBuffer(1);

				// 描画
				a_pCtx->DrawShape();
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}