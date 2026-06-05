#include "TAAPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/D3DObject/CommandList/CommandList.h"

#include "../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void Engine::Graphics::AddTAAPass(
		D3D12::PipelineStateManager* a_pPSOManager, 
		RenderGraph& a_rg, 
		const EDrawPhase& a_phase
	)
	{
		// ランタイムデータ取得
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		_spPassData->pRG = &a_rg;

		// ノードパスビルダー作成
		RenderPassNode _node = {};
		_node.name = "TemporalAccumulationPass";
		RGComputePassBuilder _cpBuilder(&_node, &a_rg);

		// ルートシグネチャセット
		_cpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso");

		// シェーダーセット
		_cpBuilder.SetShader("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso", "TemporalAccumulationPass", _spPassData->csIndex);

		// 依存関係構築

	}
}