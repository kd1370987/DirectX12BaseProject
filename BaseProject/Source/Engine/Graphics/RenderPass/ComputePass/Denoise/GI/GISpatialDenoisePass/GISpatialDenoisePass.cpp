#include "GISpatialDenoisePass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "../../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{

	void AddGISpatialDenoisePass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイムデータ作成
		struct RuntimeDate
		{
			ID3D12RootSignature* pRootSignature;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeDate>();
		_spPassData->pPSOManager = a_pPSOManager;
		_spPassData->pRG = &a_rg;
		// PSO共通化のため先にルートしぐねちゃ設定
		_spPassData->pRootSignature = a_pPSOManager->Request("Asset/Shader/Compute/Denoise/GI/GISpatialDenoiseShader.cso");

		// ダミービルダーを使ってPSOを一度のみ作成
		RenderPassNode _dummyNode = {};
		RGComputePassBuilder _psoBuilder(&_dummyNode, &a_rg);
		_psoBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Compute/Denoise/GI/GISpatialDenoiseShader.cso");
		_psoBuilder.SetShader("Asset/Shader/Compute/Denoise/GI/GISpatialDenoiseShader.cso", "GISpatialDenoiseShader", _spPassData->csIndex);
		_psoBuilder.ResolveAndCompile(a_pPSOManager);

		// ピンポン用のリソース名設定
		std::string _initialSrc = "DenoiseGI";
		std::string _tempA = "SpatialTemp_A";
		std::string _tempB = "SpatialTemp_B";
		std::string _finalDst = "FinalGI";

		int _passCount = 5;

		// ループでパスを複数回登録
		for (int _i = 0; _i < _passCount; ++_i)
		{
			// ステップサイズの計算
			int _stepSize = 1 << _i;

			// 入出力をピンポンさせるルーティング
			std::string _readGI;
			std::string _writeGI;

			if (_i == 0)
			{
				// 初回
				_readGI = _initialSrc;
				_writeGI = _tempA;
			}
			else if (_i == _passCount - 1)
			{
				// 最終回
				_readGI = (_i % 2 != 0) ? _tempA : _tempB;
				_writeGI = _finalDst;
			}
			else
			{
				// 中間パス
				_readGI = (_i % 2 != 0) ? _tempA : _tempB;
				_writeGI = (_i % 2 != 0) ? _tempB : _tempA;
			}

			// ノード作成
			RenderPassNode _node = {};
			_node.name = "GISpatialDenoisePass_Step" + std::to_string(_stepSize);
			RGComputePassBuilder _cpBuilder(&_node, &a_rg);

			// 依存関係の構築
			_cpBuilder.Read(_readGI, AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);

			_cpBuilder.Write(_writeGI, AccessType::UAV, LoadOp::Load, StoreOp::Store);

			// 実行関数の登録
			_node.executeFunc = [_spPassData, _stepSize, _readGI, _writeGI, _passName = _node.name]
			(GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				Editor::MainEditor::Instance().StartWatch(_passName);

				// オプション取得
				const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();

				auto* _pCmd = a_pCtx->GetCurrentCmdList();
				_pCmd->SetComputeRootSignature(_spPassData->pRootSignature);
				auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
				_pCmd->SetPipelineState(_pPSO);

				struct CBData
				{
					int		stepSize;		// パスごとのステップサイズ
					float	phiDepth;	// 深度の感度（小さいほどエッジを厳密に保護）
					float	phiNormal;	// 法線の感度（大きいほど法線のずれに敏感）
					float	phiColor;	// 輝度の感度（ノイズとディティールの境界制御）
				};
				const auto& _giOp = Option::OptionManager::GetInstance().GetGIOption();
				CBData _data = {};
				_data.stepSize = _stepSize;
				_data.phiDepth = _giOp.phiDepth;
				_data.phiNormal = _giOp.phiNormal;
				_data.phiColor = _giOp.phiColor;
				
				

				// 定数バッファバインド
				a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(
					_pCmd,
					0,
					_data
				);

				// SRVバインド
				a_pCtx->BindHeap();
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {
					_spPassData->pRG->GetSRVCPU(_readGI),
					_spPassData->pRG->GetSRVCPU("Depth"),
					_spPassData->pRG->GetSRVCPU("GBufferNormal")
				};
				a_pCtx->ComputeBindSRV(1, _cpuVec);

				// 出力先バインド
				a_pCtx->BindUAV(2, _spPassData->pRG->GetUAVCPU(_writeGI));

				// 実行
				UINT _countX = _winOp.windowWidth / 8;
				UINT _countY = _winOp.windowHegiht / 8;
				a_pCtx->Dispatch(_countX, _countY, 1);

				Editor::MainEditor::Instance().EndWatch(_passName);
			};

			a_rg.AddPassNode(a_phase, _node);
		}
	}
}