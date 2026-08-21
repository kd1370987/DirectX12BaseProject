#include "SkyPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"
#include "Engine/D3D12/D3D12Helper.h"
#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================================
// SkyPass
//
// 何も描かれていないピクセル(深度が far のまま残っているピクセル)を空で埋める。
//
// スカイドームのメッシュは置かない。画面の各ピクセルが「どちらを向いているか」を
// カメラの逆行列から復元し、その視線を仮想ドームへ飛ばして、交点の方向に対応する
// 正距円筒のスカイテクスチャを引く。判断は全部シェーダー側(SkyShader.hlsl)。
//
// ・スカイテクスチャとドームの形(地平線の高さ・半径・回転)・露出は、
//   シーンに置いた SceneAmbientObject の持ち物。
//   ここは GraphicsEngine 経由で受け取るだけで、シーンのことは知らない。
//   テクスチャが設定されていないシーンでは何も描かずに素通りする。
//
// ・置き場所はディファードライティングの後ろ。ライティング結果(AfterLighting)へ
//   直接色を書く。深度は読むだけで、比較はシェーダー内で行う
//   (far のまま残っているピクセルだけが空)。
//
// ・モーションベクター(GBufferVelocity)もここで書く。GBuffer を通らない以上
//   ここで書かないと空の速度は 0 のままで、カメラを振ったときに TAA が
//   「動いていない」と判断して空が尾を引く。
//   Lighting 帯で上書きするので TAA(PostProcess 帯)には空の速度が入ったものが渡る。
//   影とGIのテンポラルデノイズ(NotSort 帯)はこれより前に走るため、
//   これまでどおり GBuffer が書いた「空は0」のほうを読む。空のぶんは使わないので構わない。
//==============================================================================================
namespace Engine::Graphics
{
	namespace
	{
		// ルートパラメータ番号(SkyShader.hlsl の SKY_ROOT_SIG と合わせること)
		constexpr int  kRootCameraCB   = 0;
		constexpr int  kRootSkyCB      = 1;
		constexpr UINT kRootDepthSRV   = 2;	// レンダーグラフが張る
		constexpr UINT kRootSkyTexSRV  = 3;	// このパスが張る
		constexpr UINT kRootColorUAV   = 4;	// レンダーグラフが張る
		constexpr UINT kRootVelocityUAV = 5;	// レンダーグラフが張る
	}

	void AddSkyPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "Sky";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Compute/Sky/SkyShader.cso",
			"SkyShader",
			_csIndex
		);
		_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_cpBuilder.SetPassPSO(_csIndex);
		_cpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言
		// スカイテクスチャはレンダーグラフの管理外(リソースマネージャーの持ち物)なので、
		// 同じテーブルには混ぜられない。別のルートパラメータにして実行時に張る
		_cpBuilder.SrvTable(kRootDepthSRV).Add("Depth");

		// AfterLighting はライティングと同じ R16F で揃える(フォーマット不一致はグラフが破綻する)。
		// どちらも既に書かれている絵の上に乗せるので LoadOp は Load
		const RGResourceRef _colorRef =
			_cpBuilder.BindUAV(kRootColorUAV, "AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Load, StoreOp::Store);
		_cpBuilder.BindUAV(kRootVelocityUAV, "GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT, LoadOp::Load, StoreOp::Store);

		// コンパイル
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 定数バッファとスカイテクスチャ、そしてディスパッチだけ
		_node.executeFunc = [_colorRef](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				// スカイテクスチャが設定されていないシーンでは何も描かない。
				// ディスパッチごと飛ばすので、ライティングの結果がそのまま残る
				auto* _pSkyTex = Resource::ResourceManager::Instance().Get(a_pGE->GetSkyTexture());
				if (!_pSkyTex) return;

				const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
				auto* _pCmd = a_pCtx->GetCurrentCmdList();

				// カメラ : 視線方向の復元とモーションベクターに使う
				a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(_pCmd, kRootCameraCB, a_pGE->GetCameraData());

				// スカイ設定(シーンのアンビエントオブジェクト → 定数バッファ)
				a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<SkyData>(_pCmd, kRootSkyCB, a_pGE->GetSkyData());

				// スカイテクスチャ
				a_pCtx->ComputeBindSRV(kRootSkyTexSRV, _pSkyTex->GetSRV());

				// ★UAVバリア必須。
				// 直前の DeferredLighting も同じ AfterLighting へ UAV で書いている。
				// UAV同士は状態が変わらないのでレンダーグラフが遷移バリアを積まず、
				// バリアが無いと2つのDispatchがGPU上で並列に走ってしまう。
				// そうなると空を書いたあとからライティングの結果が同じ画素へ降ってきて、
				// 空が出たり消えたりする。
				D3D12::UAVBarrier(_pCmd, { a_res.Resource(_colorRef)->GetResource() });

				// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
				a_pCtx->Dispatch(
					(_winOp.windowWidth + 7) / 8,
					(_winOp.windowHeight + 7) / 8,
					1
				);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
