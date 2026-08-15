#include "CoCPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "../../../../../Option/OptionManager.h"

//==============================================================================
// CoCPass
//
// 深度バッファから CoC(ボケの大きさ)を作る。出力は1チャンネル(R16_FLOAT)。
// 符号で向きを持たせていて、-1 が手前側の最大ボケ、+1 が奥側の最大ボケ。
//
// ・深度をカメラからの深度(ビュー空間Z)へ直すのに逆射影行列が要るので、
//   カメラの定数バッファを受け取る。ここで depth → CoC まで済ませておけば
//   後段の DoF パスはカメラ行列を知らなくてよくなる。
// ・DoF が無効のときは 0(ボケなし)で埋める。後段が読んでも安全にしておく。
//==============================================================================
namespace Engine::Graphics
{
	void AddCoCPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		RenderPassNode _node = {};
		_node.name = "CoCPass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Compute/PostEffect/DoF/CoCShader.cso",
			"CoCShader",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=カメラCB / 1=DoF設定CB / 2=SRVテーブル / 3=UAV
		_cpBuilder.SrvTable(2).Add("Depth");

		// CoC は1チャンネル。全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(3, "CoC", DXGI_FORMAT_R16_FLOAT, LoadOp::DontCare, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// カメラ(逆射影行列で線形深度へ直すため)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(_pCmd, 0, a_pGE->GetCameraData());

			// DoF調整値(アクティブカメラの FocusParamComponent から積まれたもの)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 1, a_pGE->GetDoFData());

			// 実行
			// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
			a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHeight + 7) / 8, 1);
		};

		a_pRegistry->RegisterPass(_node);
	}
}
