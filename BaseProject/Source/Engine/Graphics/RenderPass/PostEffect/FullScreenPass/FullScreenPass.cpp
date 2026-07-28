#include "FullScreenPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../../RenderPassRegistry/RenderPassRegistry.h"

#include "../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddFullScreenPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		const std::string _shaderPath = "Asset/Shader/Compute/FullScreen/FullScreenCS.cso";

		RenderPassNode _node = {};
		_node.name = "FullScreenPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// ======================================================================
		// PSO / ルートシグネチャ
		// ======================================================================
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(_shaderPath, "FullScreenCS", _csIndex);
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.ResolveAndCompile(a_pPSOManager);
		_cpBuilder.SetPassPSO(_csIndex);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// ======================================================================
		// 依存関係とバインドの宣言
		// ======================================================================
		// 入力 : TAA(またはその後続)の最終カラー(SRV t0)
		_cpBuilder.SrvTable(0).Add("AfterTAAColor");

		// 出力 : 提示用カラー(UAV u0)
		// スワップチェインのバックバッファは UAV にできないため、いったん中間UAVへ書き、
		// このあと executeFunc でバックバッファへコピーする。
		// 全画素をCSで上書きするので LoadOp は DontCare。
		const RGResourceRef _outputRef = _cpBuilder.BindUAV(
			1, "PresentColor", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::DontCare, StoreOp::Store);

		// ======================================================================
		// 実行関数 : ディスパッチ後、中間UAVをバックバッファへコピー
		// ======================================================================
		_node.executeFunc = [_outputRef](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

				// ヒープ・ルートシグネチャ・PSO・SRV/UAV はグラフが実行前に張り終えている。
				// 画面全体を 8x8 スレッドグループでディスパッチ(端数切り上げ)。
				a_pCtx->Dispatch(
					(_winOp.windowWidth + 7) / 8,
					(_winOp.windowHeight + 7) / 8,
					1
				);

				// 中間UAV(PresentColor)をバックバッファへコピーする。
				// バックバッファはレンダーグラフ管理外なので手動でバリアを張り、
				// コピー後は元の状態へ戻してグラフ/EndFrameの状態前提を崩さない。
				ID3D12Resource* _pPresent = a_res.Resource(_outputRef)->GetResource();
				ID3D12Resource* _pBackBuffer = D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffer();

				// コピー用状態へ遷移
				a_pCtx->Transition(_pPresent, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
				a_pCtx->Transition(_pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);

				// PresentColor -> バックバッファ
				a_pCtx->ResourceCopy(_pPresent, _pBackBuffer);

				// 状態を元へ戻す(バックバッファ=RENDER_TARGET, PresentColor=UNORDERED_ACCESS)
				a_pCtx->Transition(_pBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
				a_pCtx->Transition(_pPresent, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			};

		a_pRegistry->RegisterPass(_node);
	}
}
