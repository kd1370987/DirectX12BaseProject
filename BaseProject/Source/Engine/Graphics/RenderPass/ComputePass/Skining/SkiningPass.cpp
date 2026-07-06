#include "SkiningPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "../../../../Option/OptionManager.h"

void Engine::Graphics::AddSkiningPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
{
	// ランタイム用データ
	struct RuntimeData
	{
		ID3D12RootSignature* pRootSig;
		uint8_t csIndex;
		D3D12::PipelineStateManager* pPSOManager;

	};
	auto _spPassData = std::make_shared<RuntimeData>();
	_spPassData->pPSOManager = a_pPSOManager;


	// ノード・ビルダー作成
	RenderPassNode _node = {};
	_node.name = "DeferredLighting";
	_node.phase = a_phase;
	RGComputePassBuilder _rpBuilder(&_node);

	// シェーダー
	auto* _pBlob = _rpBuilder.SetShader(
		"Asset/Shader/Compute/Lighting/DeferredLighting/DeferredLightingShader.cso",
		"DeferredLightingShader",
		_spPassData->csIndex
	);
	// ルートシグネチャ
	_spPassData->pRootSig = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

	// 依存関係構築


	// コンパイル
	_rpBuilder.ResolveAndCompile(a_pPSOManager);

	// 実行関数
	_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			
		};

	// パス登録
	_node.phase = a_phase;
	a_pRegistry->RegisterPass(_node);
}
