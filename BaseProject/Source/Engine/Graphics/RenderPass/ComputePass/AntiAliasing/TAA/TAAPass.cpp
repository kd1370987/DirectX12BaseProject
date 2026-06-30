#include "TAAPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

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
		_node.name = "TAAPass";
		RGComputePassBuilder _cpBuilder(&_node);

		// ルートシグネチャセット
		_spPassData->pRootSig = _cpBuilder.SetRootSignature(
			a_pPSOManager, 
			"Asset/Shader/Compute/AntiAliasing/TAA/TAA.cso"
		);
		// シェーダーセット
		_cpBuilder.SetShader(
			"Asset/Shader/Compute/AntiAliasing/TAA/TAA.cso",
			"TAAShader",
			_spPassData->csIndex
		);

		// 依存関係構築
		_cpBuilder.ReadSRV("AffterLighting");
		_cpBuilder.ReadSRV("HistoryTAAColor");
		_cpBuilder.ReadSRV("GBufferVelocity");
		_cpBuilder.ReadSRV("Depth");
		_cpBuilder.ReadSRV("GBufferNormal");

		_cpBuilder.WriteUAV("AffterTAAColor", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		// PSO作成
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				// オプション取得
				const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();

				// ヒープとルートシグネチャ、PSOをセット
				auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
				a_pCtx->BindHeap();
				a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
				a_pCtx->SetComputePSO(_pPso);

				// 参照SRV
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {
					_spPassData->pRG->GetCPUHandle("AffterLighting"),
					_spPassData->pRG->GetCPUHandle("HistoryTAAColor"),
					_spPassData->pRG->GetCPUHandle("GBufferVelocity"),
					_spPassData->pRG->GetCPUHandle("Depth"),
					_spPassData->pRG->GetCPUHandle("GBufferNormal")
				};
				a_pCtx->ComputeBindSRV(0, _gpuVec);

				// 出力テクスチャ設定
				a_pCtx->BindUAV(1, _spPassData->pRG->GetUAVCPU("AffterTAAColor"));

				// 実行
				UINT _countX = _winOp.windowWidth / 8;
				UINT _countY = _winOp.windowHegiht / 8;
				a_pCtx->Dispatch(_countX, _countY, 1);
			};

		a_rg.AddPassNode(a_phase, _node);
	}
	void Engine::Graphics::AddTAAPass(
		D3D12::PipelineStateManager* a_pPSOManager, 
		RenderPassRegistry* a_pRegistry, 
		const EDrawPhase& a_phase
	)
	{
		// ランタイムデータ取得
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
			
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		

		// ノードパスビルダー作成
		RenderPassNode _node = {};
		_node.name = "TAAPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// ルートシグネチャセット
		_spPassData->pRootSig = _cpBuilder.SetRootSignature(
			a_pPSOManager, 
			"Asset/Shader/Compute/AntiAliasing/TAA/TAA.cso"
		);
		// シェーダーセット
		_cpBuilder.SetShader(
			"Asset/Shader/Compute/AntiAliasing/TAA/TAA.cso",
			"TAAShader",
			_spPassData->csIndex
		);

		// 依存関係構築
		_cpBuilder.ReadSRV("AffterLighting");
		_cpBuilder.ReadSRV("HistoryTAAColor");
		_cpBuilder.ReadSRV("GBufferVelocity");
		_cpBuilder.ReadSRV("Depth");
		_cpBuilder.ReadSRV("GBufferNormal");

		_cpBuilder.WriteUAV("AffterTAAColor", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		// PSO作成
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				// オプション取得
				const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();

				// ヒープとルートシグネチャ、PSOをセット
				auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
				a_pCtx->BindHeap();
				a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
				a_pCtx->SetComputePSO(_pPso);

				// 参照SRV
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {
					a_pGE->RefRenderGraph()->GetCPUHandle("AffterLighting"),
					a_pGE->RefRenderGraph()->GetCPUHandle("HistoryTAAColor"),
					a_pGE->RefRenderGraph()->GetCPUHandle("GBufferVelocity"),
					a_pGE->RefRenderGraph()->GetCPUHandle("Depth"),
					a_pGE->RefRenderGraph()->GetCPUHandle("GBufferNormal")
				};
				a_pCtx->ComputeBindSRV(0, _gpuVec);

				// 出力テクスチャ設定
				a_pCtx->BindUAV(1, a_pGE->RefRenderGraph()->GetUAVCPU("AffterTAAColor"));

				// 実行
				UINT _countX = _winOp.windowWidth / 8;
				UINT _countY = _winOp.windowHegiht / 8;
				a_pCtx->Dispatch(_countX, _countY, 1);
			};

		_node.phase = a_phase;
		a_pRegistry->RegisterPass(_node);
	}
}