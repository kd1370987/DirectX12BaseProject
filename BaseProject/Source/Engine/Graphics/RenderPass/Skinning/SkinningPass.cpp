#include "SkinningPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"
#include "../../../Graphics/MeshBufferAllocator/MeshBufferAllocator.h"

void Engine::Graphics::AddSkinningPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
{
	// ランタイム用データ
	struct RuntimeData
	{
		ID3D12RootSignature* pRootSig;
		uint8_t csIndex;
		D3D12::PipelineStateManager* pPSOManager;

	};
	auto _spPassData = std::make_shared<RuntimeData>();
	_spPassData->pPSOManager = a_pPSOManager;


	// ノード・ビルダー作成
	RenderPassNode _node = {};
	_node.name = "SkinningPass";
	_node.phase = a_phase;
	RGComputePassBuilder _rpBuilder(&_node);

	// シェーダー
	auto* _pBlob = _rpBuilder.SetShader(
		"Asset/Shader/Compute/Skinning/Skinning.cso",
		"SkinningCS",
		_spPassData->csIndex
	);
	// ルートシグネチャ
	_spPassData->pRootSig = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

	// 依存関係構築


	// コンパイル
	_rpBuilder.ResolveAndCompile(a_pPSOManager);

	// 実行関数
	_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			auto* _pCmdList = a_pCtx->GetCurrentCmdList();
			auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);

			auto* _pMA = a_pGE->RefMeshBufferAllocator();
			if (!_pMA) return;

			// =====================================================================
			// モーションベクター用 : スキニングで上書きする前に、
			// 今のアニメ済みバッファ(=前フレームのスキニング結果)を prev バッファへ退避する。
			// これが無いと過去のスキニング座標が存在せず、変形分の速度が0に切り捨てられる。
			// =====================================================================
			{
				auto& _mainBuf = _pMA->RefAnimatedVertexBuffer();
				auto& _prevBuf = _pMA->RefPrevAnimatedVertexBuffer();

				_mainBuf.Barrier(_pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
				_prevBuf.Barrier(_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

				// スキニング対象メッシュの領域だけをコピー(バッファ全体はコピーしない)
				for (auto& _item : a_pGE->GetSkinningImtes())
				{
					const UINT64 _offsetBytes = static_cast<UINT64>(_item.animatedHandle.startIndex) * sizeof(Resource::MeshVertexFloat);
					const UINT64 _sizeBytes   = static_cast<UINT64>(_item.staticVertexHandle.count) * sizeof(Resource::MeshVertexFloat);
					_pCmdList->CopyBufferRegion(
						_prevBuf.GetResource(), _offsetBytes,
						_mainBuf.GetResource(), _offsetBytes,
						_sizeBytes
					);
				}

				// prev は GBuffer/ZPre のメッシュシェーダが SRV として読むので遷移させておく
				_prevBuf.Barrier(_pCmdList,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}

			a_pCtx->BindHeap();
			a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
			a_pCtx->SetComputePSO(_pPso);

			// バッファバリア (main を UAV へ : COPY_SOURCE から遷移)
			_pMA->RefAnimatedVertexBuffer().Barrier(_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			// メッシュ情報バインド
			a_pCtx->ComputeBindBonePalletBuffer(1);
			a_pCtx->ComputeBindSRV(2, _pMA->GetStaticVertexBuffer().GetSRV());
			a_pCtx->ComputeBindSRV(3, _pMA->GetIndexBuffer().GetSRV());
			a_pCtx->BindUAV(4, _pMA->GetAnimatedVertexBuffer().GetUAV());

			for (auto& _item : a_pGE->GetSkinningImtes())
			{
				
				struct Info
				{
					UINT vertexStart;			// 頂点のスタートインデックス
					UINT animatedVertStart;
					UINT vertexCount;			// キャラの頂点数
					UINT boneOffset;			// このキャラのボーンの開始場所
				} _info;
				_info.vertexStart = _item.staticVertexHandle.startIndex;
				_info.animatedVertStart = _item.animatedHandle.startIndex;
				_info.vertexCount = _item.staticVertexHandle.count;
				//_info.boneOffset = _item.nodePoseMat.startIndex;
				_info.boneOffset = _item.boneHandle.startIndex;
				a_pCtx->ComputeBindRootCBV(0, _info);

				UINT _x = (_info.vertexCount + 63) / 64;
				a_pCtx->Dispatch(_x, 1, 1);
			}
		};

	// パス登録
	_node.phase = a_phase;
	a_pRegistry->RegisterPass(_node);
}
