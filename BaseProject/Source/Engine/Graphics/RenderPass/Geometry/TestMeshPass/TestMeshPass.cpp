#include "TestMeshPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "../../../../D3D12/D3DObject/RootSignature/RootSignature.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"

void Engine::Graphics::AddMSTestPass(
	D3D12::PipelineStateManager* a_pPSOManager,
	RenderPassRegistry* a_pRegistry, 
	const EDrawPhase& a_phase
)
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
	auto& _msTmp = _msBuilder.CreatePSODesc("MSTestPSO",_spPassData->index);
	//_spPassData->pRootSig = _msBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/Test/TestMS.cso");
	_msBuilder.SetRootSignature(_spPassData->pRootSig);
	_msBuilder.SetMS(_msTmp, "Asset/Shader/Source/Test/TestMS.cso");
	_msBuilder.SetPS(_msTmp, "Asset/Shader/Source/Test/testPS.cso");

	// 依存関係構築
	_msBuilder.ReadDepth("Depth");

	_msBuilder.WriteRTV("TestMS",DXGI_FORMAT_R8G8B8A8_UNORM);

	// コンパイル
	_msBuilder.ResolveAndCompile(a_pPSOManager);

	// 実行関数
	_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			a_pCtx->BindHeap();
			a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);
			a_pGE->BindPSO(a_pCtx,_spPassData->index);

			// スレッドグループ(X, Y, Z)を指定してメッシュシェーダーを起動！
			// 今回のテストシェーダーは 1グループで1枚のポリゴンを作るので (1, 1, 1) でOK
			a_pCtx->DispatchMesh(1, 1, 1);
		};

	// パス登録
	a_pRegistry->RegisterPass(_node);
}
