#include "SkyPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"

#include "Engine/Option/OptionManager.h"

//==============================================================================
// SkyPass
//
// スカイ(Sky)シェーディングモデルを付けたマテリアルだけが通るパス。
//
// ・GBuffer は通らない。空はライティングの計算対象ではないので、
//   アルベドや法線を書き出しても使い道が無く、むしろ
//   ディファードライティングに拾われて余計な陰影が付いてしまう。
//   どのパスを通るかはシェーディングモデルのテーブル(Asset/ShadingModelTables/Sky)が
//   決めるので、そこで "Sky" だけを有効にしてある。
//
// ・置き場所はディファードライティングの後ろ。
//   ライティング結果(AfterLighting)へ直接色を書く。
//   ZPre も通っていないため深度は空のぶんだけ抜けたままで、
//   不透明が書いた深度に対して LESS_EQUAL で弾かれることで
//   手前のオブジェクトの裏に回る。深度は書かない(空の後ろには何も無い)。
//
// ・モーションベクター(GBufferVelocity)もここで書く。
//   GBufferPass を通らない以上ここで書かないと空の速度は0のままで、
//   カメラを振ったときに TAA が「動いていない」と判断して空が尾を引く。
//   Lighting 帯で上書きするので、TAA(PostProcess 帯)には空の速度が入ったものが渡る。
//   影とGIのテンポラルデノイズ(NotSort 帯)はこれより前に走るため、
//   これまでどおり GBuffer が書いた「空は0」のほうを読む。空のぶんは使わないので構わない。
//==============================================================================
namespace Engine::Graphics
{
	namespace
	{
		// MESHGLOBAL_ROOT_SIG の末尾に足したスカイ設定CB(b15)のルートパラメータ番号。
		// 0=カメラCB / 1〜8=SRV(t0〜t7) / 9=ルート定数 / 10=SRV(t8) / 11=ここ
		constexpr int kSkyOptionRootIndex = 11;

		// スカイ設定 → 定数バッファ
		// ※ HLSL 側 SkyOptionData(CBSkyOption.hlsli)と並びを合わせること
		struct SkyOptionData
		{
			float exposure;
			float pad[3];
		};

		SkyOptionData MakeSkyOptionCB()
		{
			const auto& _op = Option::OptionManager::GetInstance().GetSkyOption();

			SkyOptionData _cb = {};
			_cb.exposure = _op.exposure;

			return _cb;
		}
	}

	void AddSkyPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			Handle<ID3D12RootSignature> rootSigHandle = {};
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "Sky";
		_node.phase = a_phase;
		RGMeshShaderPassBuilder _msBuilder(&_node);

		// 依存関係構築
		// 深度は不透明が書いたものをそのまま使う(読み取り専用)
		_msBuilder.ReadDepth("Depth");

		// 宣言順がそのまま SV_Target の番号になる。
		// AfterLighting はライティングと同じ R16F で揃える(フォーマット不一致はグラフが破綻する)
		_msBuilder.WriteRTV("AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Load, StoreOp::Store);
		_msBuilder.WriteRTV("GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT, LoadOp::Load, StoreOp::Store);

		// シェーダー関係セット : メッシュの展開は GBuffer と同じものを使い回す
		auto _guidMS = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/UberMS.cso");
		auto _msHandle = Resource::ResourceManager::Instance().LoadImmediate<Resource::Shader>(_guidMS);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Skinned, _msHandle);

		auto _guidAS = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/TestAS.cso");
		auto _asHandle = Resource::ResourceManager::Instance().LoadImmediate<Resource::Shader>(_guidAS);
		_node.pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Static, _asHandle);
		_node.pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Skinned, _asHandle);

		// ルートシグネチャセット
		_spPassData->rootSigHandle = a_pPSOManager->Request("Asset/Shader/Source/Mesh/UberMS.cso");

		// 深度テスト設定
		_node.pipelineBuilder.SetDepthConfig(
			true,								// 深度テスト有効
			false,								// 書き込み無効
			D3D12_COMPARISON_FUNC_LESS_EQUAL	// ZPre を通っていないので手前のものに負ける形で比較する
		);

		// カリング設定 : モデル側の向きに任せるので既定の背面カリングのまま
		_node.pipelineBuilder.SetCullMode(D3D12_CULL_MODE_BACK);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				a_pCtx->BindCopyHeapAndSumplerBindLess();
				a_pCtx->SetGraphicsRootSignature(_spPassData->rootSigHandle);

				a_pCtx->BindCamera();
				a_pCtx->BindMeshInstance();
				a_pCtx->BindMeshlet();

				// スカイ設定(オプション → 定数バッファ)
				a_pCtx->GraphicsBindRootCBV(kSkyOptionRootIndex, MakeSkyOptionCB());

				a_pCtx->DrawQueueDispathMesh(a_res.PassIndex());
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
