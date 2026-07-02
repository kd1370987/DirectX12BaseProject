#include "GBufferPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Graphics
{
	void AddGBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
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
		_node.name = "GBuffer";
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/GBufferShader/GBufferVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		// 依存関係構築
		//_rpBuilder.Readde("Depth", AccessType::Depth_Read, LoadOp::Load, StoreOp::Store);
		_rpBuilder.ReadDepth("Depth");

		//_rpBuilder.Write("GBufferAlbedo", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.WriteRTV("GBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_rpBuilder.WriteRTV("GBufferNormal", DXGI_FORMAT_R16G16_FLOAT);
		_rpBuilder.WriteRTV("GBufferMaterial", DXGI_FORMAT_R8G8B8A8_UNORM);
		_rpBuilder.WriteRTV("GBufferEmissiv", DXGI_FORMAT_R8G8B8A8_UNORM);
		_rpBuilder.WriteRTV("GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("GBufferStatic", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		auto& _aPso = _rpBuilder.CreatePSODesc("GBufferAnimation", _spPassData->animationIndex);
		_rpBuilder.SetVS(_aPso, "Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso", D3D12::Input::AnimationInputLayout);
		_rpBuilder.SetPS(_aPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_aPso.DepthEnable(true);
		_aPso.DepthWriteMask(false);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			a_pCtx->BindHeap();
			a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

			CameraData _cbCam = a_pGE->GetCameraData();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();
			a_pCtx->BindCB()->BindAndAttachDataRootCBV<CameraData>(_pCmd, 0, _cbCam);
			a_pCtx->BindInstanceBuffer(2);
			a_pCtx->BindSubsetBuffer(3);
			a_pCtx->BindBonePalletBuffer(4);
			
			// 描画
			a_pGE->DrawQueue(a_pCtx, a_passIndex);
		};

		// パス登録
		a_rg.AddPassNode(a_phase, _node);
	}
	void AddGBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
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
		_node.name = "GBuffer";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/GBufferShader/GBufferVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		// 依存関係構築
		//_rpBuilder.Readde("Depth", AccessType::Depth_Read, LoadOp::Load, StoreOp::Store);
		_rpBuilder.ReadDepth("Depth");

		//_rpBuilder.Write("GBufferAlbedo", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.WriteRTV("GBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_rpBuilder.WriteRTV("GBufferNormal", DXGI_FORMAT_R16G16_FLOAT);
		_rpBuilder.WriteRTV("GBufferMaterial", DXGI_FORMAT_R8G8B8A8_UNORM);
		_rpBuilder.WriteRTV("GBufferEmissiv", DXGI_FORMAT_R8G8B8A8_UNORM);
		_rpBuilder.WriteRTV("GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("GBufferStatic", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		auto& _aPso = _rpBuilder.CreatePSODesc("GBufferAnimation", _spPassData->animationIndex);
		_rpBuilder.SetVS(_aPso, "Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso", D3D12::Input::AnimationInputLayout);
		_rpBuilder.SetPS(_aPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_aPso.DepthEnable(true);
		_aPso.DepthWriteMask(false);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				CameraData _cbCam = a_pGE->GetCameraData();
				auto* _pCmd = a_pCtx->GetCurrentCmdList();
				a_pCtx->BindCB()->BindAndAttachDataRootCBV<CameraData>(_pCmd, 0, _cbCam);
				a_pCtx->BindInstanceBuffer(2);
				a_pCtx->BindSubsetBuffer(3);
				a_pCtx->BindBonePalletBuffer(4);

				// 描画
				a_pGE->DrawQueue(a_pCtx, a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
	void AddMeshGBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t index;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード
		RenderPassNode _node = {};
		_node.name = "MSTestPass";
		_node.phase = a_phase;

		// 空のルートシグネチャ作成
		D3D12::RootSignatureDesc _rootSigDesc = {};
		_rootSigDesc.isUseStaticSampler = false;
		_spPassData->pRootSig = a_pPSOManager->Request(_rootSigDesc);

		// PSO作成
		RGMeshShaderPassBuilder _msBuilder(&_node);
		auto& _msTmp = _msBuilder.CreatePSODesc("MSTestPSO", _spPassData->index);
		//_spPassData->pRootSig = _msBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/Test/TestMS.cso");
		_msBuilder.SetRootSignature(_spPassData->pRootSig);
		_msBuilder.SetMS(_msTmp, "Asset/Shader/Source/Test/TestMS.cso");
		_msBuilder.SetPS(_msTmp, "Asset/Shader/Source/Test/testPS.cso");

		// 依存関係構築
		_msBuilder.ReadDepth("Depth");

		_msBuilder.WriteRTV("TestMS", DXGI_FORMAT_R8G8B8A8_UNORM);

		// コンパイル
		_msBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);
				a_pGE->BindPSO(a_pCtx, _spPassData->index);

				// スレッドグループ(X, Y, Z)を指定してメッシュシェーダーを起動！
				// 今回のテストシェーダーは 1グループで1枚のポリゴンを作るので (1, 1, 1) でOK
				a_pCtx->DispatchMesh(1, 1, 1);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}