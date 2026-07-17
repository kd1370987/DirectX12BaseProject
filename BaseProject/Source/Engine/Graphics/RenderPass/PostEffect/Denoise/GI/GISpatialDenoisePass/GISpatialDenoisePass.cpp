#include "GISpatialDenoisePass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "../../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddGISpatialDenoisePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		const std::string _shaderPath = "Asset/Shader/Compute/Denoise/GI/GISpatialDenoiseShader.cso";

		// PSO共通化のため先にルートシグネチャ設定
		a_pPSOManager->Request(_shaderPath);

		// ダミービルダーを使ってPSOを一度のみ作成（全ステップで使い回すため）
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		RenderPassNode _dummyNode = {};
		RGComputePassBuilder _psoBuilder(&_dummyNode);
		auto* _pBlob = _psoBuilder.SetShader(_shaderPath, "GISpatialDenoiseShader", _csIndex);
		_psoBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_psoBuilder.ResolveAndCompile(a_pPSOManager);

		// ピンポン用のリソース名設定
		const std::string _initialSrc = "DenoiseGI";
		const std::string _tempA = "SpatialTemp_A";
		const std::string _tempB = "SpatialTemp_B";
		const std::string _finalDst = "FinalGI";

		const int _passCount = 5;

		// ======================================================================
		// ループでパスを複数回登録（AとBをピンポンさせる）
		// ======================================================================
		for (int _i = 0; _i < _passCount; ++_i)
		{
			// ステップサイズの計算 (1, 2, 4, 8, 16)
			const int _stepSize = 1 << _i;

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
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// 全ステップで同じルートシグネチャとPSOを使い回す
			_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
			_cpBuilder.SetPassPSO(_csIndex);
			_cpBuilder.SetHeapMode(ERGHeapMode::Default);

			// 依存関係とバインドの宣言（宣言順 = t0～t2）
			// ※レンダーグラフはここで宣言した順序とバージョンを完璧に追跡します
			_cpBuilder.SrvTable(1)
				.Add(_readGI)
				.Add("Depth")
				.Add("GBufferNormal");

			// 中間バッファは毎パス上書きする
			_cpBuilder.BindUAV(2, _writeGI, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Load, StoreOp::Store, 0.5f);

			// ==================================================================
			// 実行関数 : ステップサイズ由来の定数バッファとディスパッチのみ
			// ==================================================================
			_node.executeFunc = [_stepSize](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
				{
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
					const auto& _giOp = Option::OptionManager::GetInstance().GetGIOption();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();

					// 定数バッファデータの設定
					struct CBData
					{
						int		stepSize;	// パスごとのステップサイズ
						float	phiDepth;	// 深度の感度（小さいほどエッジを厳密に保護）
						float	phiNormal;	// 法線の感度（大きいほど法線のずれに敏感）
						float	phiColor;	// 輝度の感度（ノイズとディティールの境界制御）
					};

					CBData _data = {};
					_data.stepSize  = _stepSize;
					_data.phiDepth  = _giOp.phiDepth;
					_data.phiNormal = _giOp.phiNormal;
					_data.phiColor  = _giOp.phiColor;

					// 定数バッファバインド
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _data);

					// 実行
					a_pCtx->Dispatch(_winOp.windowWidth / 2 / 8, _winOp.windowHegiht / 2 / 8, 1);
				};

			a_pRegistry->RegisterPass(_node);
		}
	}
}
