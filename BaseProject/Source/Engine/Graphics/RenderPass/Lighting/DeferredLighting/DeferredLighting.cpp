#include "DeferredLighting.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddDeferredLighting(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "DeferredLighting";
		_node.phase = a_phase;
		RGComputePassBuilder _rpBuilder(&_node);

		// シェーダー
		uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
		auto* _pBlob = _rpBuilder.SetShader(
			"Asset/Shader/Source/Lighting/Deferred/DeferredLightingShader.cso",
			"DeferredLightingShader",
			_csIndex
		);
		// ルートシグネチャ
		_rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
		_rpBuilder.SetHeapMode(ERGHeapMode::Default);

		// 依存関係とバインドの宣言。
		// ここでの宣言順がそのままシェーダのレジスタ順（t0～t6）になる
		_rpBuilder.SrvTable(2)
			.Add("GBufferAlbedo")
			.Add("GBufferNormal")
			.Add("GBufferMaterial")
			.Add("GBufferEmissiv")
			.Add("Depth")
			.Add("ShadowDenoised")	// テンポラル→スペースデノイズ後の影
			.Add("FinalFullRay");

		// HDR : ライティング結果は1.0を超えるためR16Fで保持する(R8だとここで白飛びがクランプされる)
		_rpBuilder.BindUAV(3, "AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Clear, StoreOp::Store);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数 : 静的に宣言しきれない定数バッファとディスパッチだけ
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// カメラ
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(_pCmd, 0, a_pGE->GetCameraData());

			// アンビエントカラー
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(_pCmd, 1, a_pGE->GetAmbientData());

			// ライティング調整値(オプション → 定数バッファ)
			// HLSL の LightingOptionData と同じレイアウトで詰める。
			// ルートパラメータ番号4 = DEFERRED_ROOT_SIG 末尾に足した RS_LIGHTING_OPTION_CB。
			struct LightingOptionCB
			{
				float giIntensity;
				float directionalIntensity;
				float dielectricF0;
				float pad;
			};
			const auto& _lightOp = Option::OptionManager::GetInstance().GetLightingOption();
			LightingOptionCB _lightCB = {};
			_lightCB.giIntensity          = _lightOp.giIntensity;
			_lightCB.directionalIntensity = _lightOp.directionalIntensity;
			_lightCB.dielectricF0         = _lightOp.dielectricF0;
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 4, _lightCB);

			// ライト
			//
			// ライト配列はレンダーグラフの資源ではないので、SrvTable(2) の宣言には乗らない。
			// GraphicsEngine が Execute() で今フレームぶんを詰め直したものを、ここで直接張る。
			// ルートパラメータ番号5/6 = DEFERRED_ROOT_SIG 末尾に足した t7-t8 と b12。
			//
			// テーブルは並べた順にディスクリプタが入るので、t7 = ポイント / t8 = 平行光 の順を崩さないこと
			const auto& _frameLight = a_pGE->GetFrameLightData();
			const D3D12_CPU_DESCRIPTOR_HANDLE _lightSrvArr[] = {
				D3D12::DescriptorHeapManager::Instance().GetCPU(_frameLight.plBuffer.GetSRV()),
				D3D12::DescriptorHeapManager::Instance().GetCPU(_frameLight.dlBuffer.GetSRV()),
			};
			a_pCtx->ComputeBindSRV(5, _lightSrvArr);

			// ライト数
			// StructuredBuffer は要素数を持たないので、ループの上限をCBで渡す。
			// HLSL の LightCountData と同じレイアウトで詰める
			struct LightCountCB
			{
				uint32_t directionalNum;
				uint32_t pointNum;
				uint32_t pad[2];
			};
			LightCountCB _countCB = {};
			_countCB.directionalNum = _frameLight.dlCount;
			_countCB.pointNum       = _frameLight.plCount;
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 6, _countCB);

			// 実行
			// 切り上げ : 解像度が8の倍数でないと末尾タイルが実行されず端が処理されない
			a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHeight + 7) / 8, 1);
		};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
