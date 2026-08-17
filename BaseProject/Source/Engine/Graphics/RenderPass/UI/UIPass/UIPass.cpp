#include "UIPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"

#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddUIPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			Handle<ID3D12RootSignature> rootSigHandle = {};
			uint8_t staticIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード作成
		RenderPassNode _node = {};
		_node.name = "UIPass";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// 依存宣言
		// UIはシーンの最終カラー(AfterTAAColor)の上に重ねて描く。
		// DebugLinePassと同じターゲット/フォーマットに合わせ、LoadOp::Load で既存のシーンを
		// 保持したまま上書き合成する。こうすることで、最後の FullScreenPass がこのカラーを
		// バックバッファへ提示してくれる。専用の"UI"ターゲットは誰も読まず画面に出ないため使わない。
		_rpBuilder.WriteRTV("AfterTAAColor", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Load, StoreOp::Store);

		// UIシェーダは ResourceDescriptorHeap[texIndex] でテクスチャを直接引くため、
		// ルートシグネチャに CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED フラグを持つ。
		// このフラグ付きルートシグネチャを張った"後"にディスクリプタヒープを差し替えると
		// D3D12エラー #1420 になる。グラフにヒープをルートシグネチャより前へ張らせるため、
		// ヒープモードを宣言しておく(executeFunc側での手動ヒープバインドは不要になる)。
		_rpBuilder.SetHeapMode(ERGHeapMode::BindlessWithSampler);

		// PSO作成
		auto& _pso = _rpBuilder.CreatePSODesc("UIPso", _spPassData->staticIndex);
		auto* _pBlob = _rpBuilder.SetVS(_pso, "Asset/Shader/Source/UI/UIVS.cso", D3D12::Input::gParticleInputLayout);
		_rpBuilder.SetPS(_pso, "Asset/Shader/Source/UI/UIPS.cso");
		_spPassData->rootSigHandle = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		// UIは深度を使わない。DSVを張らないので、PSO側の深度フォーマットもUNKNOWNにする。
		// (でないと #615 DEPTH_STENCIL_FORMAT_MISMATCH_PIPELINE_STATE になる)
		_pso.DepthEnable(false);
		_pso.StencilEnable(false);

		// UIクアッドの巻き順(インデックス {0,1,2,3,1,0})は表裏が混在しており、
		// デフォルトの裏面カリング(CULL_BACK)だと三角形が消えてVS出力が真っ黒になる。
		// スクリーン向きの板ポリなのでカリング自体を無効化する。
		_pso.CullMode(D3D12_CULL_MODE_NONE);

		// 半透明UIをシーンへアルファ合成する。
		_pso.BlendEnable(true);
		_pso.SrcBlend(D3D12_BLEND_SRC_ALPHA);
		_pso.DestBlend(D3D12_BLEND_INV_SRC_ALPHA);
		_pso.BlendOp(D3D12_BLEND_OP_ADD);
		_pso.SrcBlendAlpha(D3D12_BLEND_ONE);
		_pso.DestBlendAlpha(D3D12_BLEND_INV_SRC_ALPHA);
		_pso.BlendOpAlpha(D3D12_BLEND_OP_ADD);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 確定したPSOをグラフに自動セットさせる。
		// これが無いと前のパスのPSOのまま描画されてしまう(UI用PSOが一切バインドされない)。
		_rpBuilder.SetPassPSO(_spPassData->staticIndex);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				// ヒープ・ルートシグネチャ・PSOはグラフが実行前に張り終えている。
				// ここでは三角形トポロジに切り替え、UIインスタンスバッファ(t0)を張って描くだけ。
				// (DrawPolygonInstancing はトポロジを設定しないため、ここで明示する)
				a_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				a_pCtx->BindUIBuffer(0);
				a_pCtx->DrawUI();
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
