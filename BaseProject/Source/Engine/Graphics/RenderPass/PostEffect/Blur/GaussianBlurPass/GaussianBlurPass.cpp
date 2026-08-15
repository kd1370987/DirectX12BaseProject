#include "GaussianBlurPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================
// GaussianBlurPass
//
// 入力テクスチャをボカして、指定した解像度スケールの出力テクスチャへ書き出す汎用パス。
// 入出力の解像度が違ってよいので、これ1つで縮小にも拡大にも使える。
//
// ・シェーダーは出力側の画素からUVを引いて入力をサンプリングするだけなので、
//   スケールの違いはサンプラーのバイリニアが吸収する。
// ・ただしサンプル位置の刻み幅(1テクセルぶんのUV)は入力の解像度で決まるので、
//   それだけは定数バッファで渡してやる必要がある。
//   出力の刻みで代用すると、拡大時にオフセットが入力の1テクセル未満になって
//   ブラーが効かなくなる。
//==============================================================================
namespace Engine::Graphics
{
	namespace
	{
		// レンダーグラフがテクスチャを確保するときと同じ式で解像度を求める。
		// (RenderGraph::CreateResource が windowWidth * texScale を切り捨てている)
		// ここがズレるとディスパッチ数や1テクセルぶんのUVが実際のテクスチャと食い違う
		void CalcScaledSize(float a_scale, UINT& a_outWidth, UINT& a_outHeight)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

			a_outWidth  = static_cast<UINT>(_winOp.windowWidth * a_scale);
			a_outHeight = static_cast<UINT>(_winOp.windowHeight * a_scale);

			// 0除算・0ディスパッチ避け
			if (a_outWidth == 0)  a_outWidth = 1;
			if (a_outHeight == 0) a_outHeight = 1;
		}
	}

	void AddGaussianBlurPass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase,
		const std::string& a_passName,
		const std::string& a_srcName,
		const std::string& a_dstName,
		float a_srcScale,
		float a_dstScale,
		float a_sigma,
		int a_tapRadius,
		DXGI_FORMAT a_format
	)
	{
		RenderPassNode _node = {};
		_node.name = a_passName;
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		// PSOはデスクのハッシュでキャッシュされるので、何回登録しても実体は1つで済む
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Compute/PostEffect/Blur/GaussianBlurShader.cso",
			"GaussianBlurShader",
			_csIndex
		);
		// ルートシグネチャ
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// ルートパラメータ : 0=ブラー設定CB / 1=SRVテーブル(t0) / 2=UAV
		_cpBuilder.SrvTable(1).Add(a_srcName);

		// 出力。全画素を書き潰すので LoadOp は DontCare
		_cpBuilder.BindUAV(2, a_dstName, a_format, LoadOp::DontCare, StoreOp::Store, a_dstScale);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとディスパッチだけ
		// 解像度はウィンドウサイズから毎フレーム引き直す(スケールだけ持ち回る)
		_node.executeFunc =
			[a_srcScale, a_dstScale, a_sigma, a_tapRadius]
			(GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			UINT _srcWidth = 0, _srcHeight = 0;
			UINT _dstWidth = 0, _dstHeight = 0;
			CalcScaledSize(a_srcScale, _srcWidth, _srcHeight);
			CalcScaledSize(a_dstScale, _dstWidth, _dstHeight);

			// ブラー設定
			GaussianBlurCB _blurCB = {};
			_blurCB.srcTexelSize = { 1.0f / static_cast<float>(_srcWidth), 1.0f / static_cast<float>(_srcHeight) };
			_blurCB.sigma        = a_sigma;
			_blurCB.tapRadius    = a_tapRadius;
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _blurCB);

			// 実行
			// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
			a_pCtx->Dispatch((_dstWidth + 7) / 8, (_dstHeight + 7) / 8, 1);
		};

		a_pRegistry->RegisterPass(_node);
	}
}
