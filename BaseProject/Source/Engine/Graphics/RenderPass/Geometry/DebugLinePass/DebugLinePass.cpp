#include "DebugLinePass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"

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
			Handle<ID3D12RootSignature> rootSigHandle = {};
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

		// HDR : AfterTAAColor は R16F に統一(TAA と同フォーマットにする)
		_rpBuilder.WriteRTV("AfterTAAColor", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Load, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("DebugLinePSO", _spPassData->staticPsoIndex);
		// このVSは SV_VertexID / SV_InstanceID だけで頂点を生成し、頂点バッファを読まない。
		// StaticLayout を宣言すると IA が頂点バッファを要求し、直前のパスが残したバッファ
		// (パーティクルのクアッド等)を読もうとして #210 警告が出るため、空レイアウトを使う。
		auto* _pBlob = _rpBuilder.SetVS(_sPso, "Asset/Shader/Source/Debug/DebugLine/DebugLineVS.cso", D3D12::Input::gEmptyLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/Debug/DebugLine/DebugLinePS.cso");
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		_spPassData->rootSigHandle = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		_sPso.desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				// ヒープ・ルートシグネチャ・PSOセット
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->rootSigHandle);
				a_pCtx->SetGraphicPSO(_spPassData->staticPsoIndex);

				a_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

				// カメラのバインド
				a_pCtx->BindCamera();

				// デバッグライン描画用構造体バッファバインド
				a_pCtx->BindGraphicsDebugLineBuffer(1);

				// 描画
				a_pCtx->DrawShape();
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}