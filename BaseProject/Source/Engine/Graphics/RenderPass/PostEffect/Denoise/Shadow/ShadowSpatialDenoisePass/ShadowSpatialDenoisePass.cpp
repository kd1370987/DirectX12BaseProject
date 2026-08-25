#include "ShadowSpatialDenoisePass.h"

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
	void AddShadowSpatialDenoisePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		const std::string _shaderPath = "Asset/Shader/Source/Lighting/Denoise/Shadow/ShadowSpatialDenoiseShader.cso";

		// PSO共通化のため先にルートシグネチャ設定
		a_pPSOManager->Request(_shaderPath);

		// ダミービルダーを使ってPSOを一度のみ作成（全ステップで使い回すため）
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		RenderPassNode _dummyNode = {};
		RGComputePassBuilder _psoBuilder(&_dummyNode);
		auto* _pBlob = _psoBuilder.SetShader(_shaderPath, "ShadowSpatialDenoiseShader", _csIndex);
		_psoBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_psoBuilder.ResolveAndCompile(a_pPSOManager);

		// ピンポン用のリソース名設定
		const std::string _initialSrc = "AfterDLShadowTempAccumu";	// テンポラル蓄積後の影
		const std::string _tempA = "ShadowSpatialTemp_A";
		const std::string _tempB = "ShadowSpatialTemp_B";
		const std::string _finalDst = "ShadowDenoised";				// ライティングが読む最終出力

		const int _passCount = 4; // ステップサイズ 1,2,4,8

		// ======================================================================
		// ループでパスを複数回登録（AとBをピンポンさせる）
		// ======================================================================
		for (int _i = 0; _i < _passCount; ++_i)
		{
			// ステップサイズの計算 (1, 2, 4, 8, ...)
			const int _stepSize = 1 << _i;

			const bool _isFirst = (_i == 0);
			const bool _isLast = (_i == _passCount - 1);

			// 入出力をピンポンさせるルーティング(1パスのみなら入力→出力へ直結)
			const std::string _readShadow = _isFirst ? _initialSrc : (((_i - 1) % 2 == 0) ? _tempA : _tempB);
			const std::string _writeShadow = _isLast ? _finalDst : ((_i % 2 == 0) ? _tempA : _tempB);

			// ノード作成
			RenderPassNode _node = {};
			_node.name = "ShadowSpatialDenoisePass_Step" + std::to_string(_stepSize);
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// 全ステップで同じルートシグネチャとPSOを使い回す
			_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
			_cpBuilder.SetPassPSO(_csIndex);
			_cpBuilder.SetHeapMode(ERGHeapMode::Default);

			// 依存関係とバインドの宣言（宣言順 = t0～t2）
			_cpBuilder.SrvTable(1)
				.Add(_readShadow)
				.Add("Depth")
				.Add("GBufferNormal");

			// 中間バッファは毎パス上書きする（影はGBufferと同解像度なのでスケールは等倍）
			_cpBuilder.BindUAV(2, _writeShadow, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Load, StoreOp::Store);

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
						int		stepSize;
						float	phiDepth;
						float	phiNormal;
						float	phiColor;
					};

					// 影専用の調整値はまだ無いため、GIのスペースデノイズ設定を流用する
					CBData _data = {};
					_data.stepSize  = _stepSize;
					_data.phiDepth  = _giOp.phiDepth;
					_data.phiNormal = _giOp.phiNormal;
					_data.phiColor  = _giOp.phiColor;

					// 定数バッファバインド
					a_pCtx->ComputeBindRootCBV(0, _data);

					// 実行(GBufferと同解像度 = フル解像度)
					// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
					a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHeight + 7) / 8, 1);
				};

			a_pRegistry->RegisterPass(_node);
		}
	}
}
