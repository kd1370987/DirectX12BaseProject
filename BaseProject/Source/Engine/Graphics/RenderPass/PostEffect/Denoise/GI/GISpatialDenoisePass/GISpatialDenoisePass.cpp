#include "GISpatialDenoisePass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "../../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddGISpatialDenoisePass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase,
		const std::string& a_passName,
		const std::string& a_inputRes,
		const std::string& a_outputRes,
		int a_passCount,
		DXGI_FORMAT a_format)
	{
		if (a_passCount <= 0) return;

		const std::string _shaderPath = "Asset/Shader/Compute/Denoise/GI/GISpatialDenoiseShader.cso";

		// PSO共通化のため先にルートシグネチャ設定
		a_pPSOManager->Request(_shaderPath);

		// ダミービルダーを使ってPSOを一度のみ作成（全ステップで使い回すため）
		// ※PSO/ルートシグネチャはハッシュでキャッシュされるので、本関数を複数回呼んでも重複生成されない
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		RenderPassNode _dummyNode = {};
		RGComputePassBuilder _psoBuilder(&_dummyNode);
		auto* _pBlob = _psoBuilder.SetShader(_shaderPath, "GISpatialDenoiseShader", _csIndex);
		_psoBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_psoBuilder.ResolveAndCompile(a_pPSOManager);

		// ピンポン用のリソース名設定
		// 中間バッファ名もパス名から生成し、本関数を複数回登録しても衝突しないようにする
		const std::string _initialSrc = a_inputRes;
		const std::string _tempA = a_passName + "_Temp_A";
		const std::string _tempB = a_passName + "_Temp_B";
		const std::string _finalDst = a_outputRes;

		const int _passCount = a_passCount;

		// ======================================================================
		// ループでパスを複数回登録（AとBをピンポンさせる）
		// ======================================================================
		for (int _i = 0; _i < _passCount; ++_i)
		{
			// ステップサイズの計算 (1, 2, 4, 8, 16, ...)
			const int _stepSize = 1 << _i;

			const bool _isFirst = (_i == 0);
			const bool _isLast = (_i == _passCount - 1);

			// 入出力をピンポンさせるルーティング。
			// 1パスのみ(_passCount==1)の場合は入力→出力へ直結する。
			//   初回      : 入力から読む
			//   それ以外  : 直前パスが書いた中間バッファ(iが奇数→A / 偶数→B)から読む
			//   最終回    : 出力へ書く / それ以外は中間バッファ(iが偶数→A / 奇数→B)へ書く
			const std::string _readGI = _isFirst ? _initialSrc : (((_i - 1) % 2 == 0) ? _tempA : _tempB);
			const std::string _writeGI = _isLast ? _finalDst : ((_i % 2 == 0) ? _tempA : _tempB);

			// ノード作成
			RenderPassNode _node = {};
			_node.name = a_passName + "_Step" + std::to_string(_stepSize);
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
			_cpBuilder.BindUAV(2, _writeGI, a_format, LoadOp::Load, StoreOp::Store, 0.5f);

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
					// 切り上げ : ハーフ解像度(例:1080→540)は540/8=67で切り捨てられ下端が処理されないため
					a_pCtx->Dispatch((_winOp.windowWidth / 2 + 7) / 8, (_winOp.windowHeight / 2 + 7) / 8, 1);
				};

			a_pRegistry->RegisterPass(_node);
		}
	}
}
