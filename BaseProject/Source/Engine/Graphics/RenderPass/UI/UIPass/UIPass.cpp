#include "UIPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"

#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddUIPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード作成
		RenderPassNode _node = {};
		_node.name = "UIPass";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// 依存宣言
		_rpBuilder.WriteRTV("UI", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		// PSO作成
		auto& _pso = _rpBuilder.CreatePSODesc("UIPso", _spPassData->staticIndex);
		auto* _pBlob = _rpBuilder.SetVS(_pso, "Asset/Shader/Source/UI/UIVS.cso", D3D12::Input::gParticleInputLayout);
		_rpBuilder.SetPS(_pso, "Asset/Shader/Source/UI/UIPS.cso");
		_spPassData->pRootSig = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				a_pCtx->BindCopyHeapAndSumplerBindLess();
				a_pCtx->BindUIBuffer(0);
				a_pCtx->DrawUI();
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
