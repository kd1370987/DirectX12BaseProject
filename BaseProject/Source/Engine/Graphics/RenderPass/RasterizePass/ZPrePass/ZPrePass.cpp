#include "ZPrePass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Graphics
{
	void AddZPrePass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
			uint8_t animationIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "ZPre";
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/ZPreShader/ZPreVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/ZPreShader/ZPreVS.cso");

		// 依存関係構築
		_rpBuilder.WriteDepth("Depth", DXGI_FORMAT_R32_TYPELESS, LoadOp::Clear, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("ZPreStatic", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/ZPreShader/ZPreVS.cso", D3D12::Input::StaticLayout);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		auto& _aPso = _rpBuilder.CreatePSODesc("ZPreAnimation", _spPassData->animationIndex);
		_rpBuilder.SetVS(_aPso, "Asset/Shader/Source/ZPreShader/AnimationZPreVS.cso", D3D12::Input::AnimationInputLayout);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			a_pCtx->BindHeap();
			a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

			CameraData _cbCam = a_pGE->GetCameraData();
			a_pCtx->GraphicsBindRootCBV(0, _cbCam);
			a_pCtx->BindInstanceBuffer(2);
			a_pCtx->BindSubsetBuffer(3);
			a_pCtx->BindBonePalletBuffer(4);
			
			// 描画
			a_pGE->DrawQueue(a_pCtx, a_passIndex);
		};

		// パス登録
		a_rg.AddPassNode(a_phase, _node);
	}

	void AddZPrePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
			uint8_t animationIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "ZPre";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/ZPreShader/ZPreVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/ZPreShader/ZPreVS.cso");

		// 依存関係構築
		_rpBuilder.WriteDepth("Depth", DXGI_FORMAT_R32_TYPELESS, LoadOp::Clear, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("ZPreStatic", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/ZPreShader/ZPreVS.cso", D3D12::Input::StaticLayout);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		auto& _aPso = _rpBuilder.CreatePSODesc("ZPreAnimation", _spPassData->animationIndex);
		_rpBuilder.SetVS(_aPso, "Asset/Shader/Source/ZPreShader/AnimationZPreVS.cso", D3D12::Input::AnimationInputLayout);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				CameraData _cbCam = a_pGE->GetCameraData();
				a_pCtx->GraphicsBindRootCBV(0, _cbCam);
				a_pCtx->BindInstanceBuffer(2);
				a_pCtx->BindSubsetBuffer(3);
				a_pCtx->BindBonePalletBuffer(4);

				// 描画
				a_pGE->DrawQueue(a_pCtx, a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
	
}
