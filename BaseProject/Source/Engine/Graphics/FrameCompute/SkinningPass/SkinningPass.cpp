#include "SkinningPass.h"

#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"
#include "Engine/Graphics/MeshBufferAllocator/MeshBufferAllocator.h"
#include "Engine/Resource/Data/Shader/IO/ShaderIO.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace
{
	// スキニングはカメラに依存せず、フレームに1回でよい計算なので
	// レンダーグラフのパスにはしていない。
	// ルートシグネチャとPSOは1度だけ用意して、ここへ置いておく
	struct SkinningRuntime
	{
		Engine::Handle<ID3D12RootSignature> rootSigHandle = {};
		uint8_t csIndex = 255;
		Engine::D3D12::PipelineStateManager* pPSOManager = nullptr;
	};
	SkinningRuntime g_skinning = {};
}

void Engine::Graphics::SetupSkinning(D3D12::PipelineStateManager* a_pPSOManager)
{
	if (!a_pPSOManager) return;
	g_skinning.pPSOManager = a_pPSOManager;

	// シェーダーからルートシグネチャとコンピュートPSOを起こす
	auto _csHandle = Resource::ShaderIO::Request("Asset/Shader/Source/Geometry/Skinning/Skinning.cso");
	auto* _pShader = Resource::ResourceManager::Instance().Ref(_csHandle);
	if (!_pShader || !_pShader->Get()) return;

	g_skinning.rootSigHandle = a_pPSOManager->Request(_pShader->Get());

	D3D12::ComputePipelineDesc _desc = {};
	_desc.SetName("SkinningCS");
	_desc.desc.CS.pShaderBytecode = _pShader->Get()->GetBufferPointer();
	_desc.desc.CS.BytecodeLength = _pShader->Get()->GetBufferSize();
	_desc.SetRootSignature(a_pPSOManager->GetRootSignature(g_skinning.rootSigHandle));

	g_skinning.csIndex = static_cast<uint8_t>(a_pPSOManager->RequestHandle(_desc).GetIndex());
}

// 中身は旧 SkinningPass の実行関数をそのまま移したもの
void Engine::Graphics::ExecuteSkinning(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
{
	if (!a_pGE || !a_pCtx) return;
	if (!g_skinning.pPSOManager) return;

	auto* _spPassData = &g_skinning;
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
			a_pCtx->SetComputeRootSignature(_spPassData->rootSigHandle);
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
				// プールの添字ではなくボーンパレット(GPU)上の位置。
				// ワールドごとの土台が足してあるので、シーンを重ねても他人のボーンを踏まない
				_info.boneOffset = _item.boneBufferStart;
				a_pCtx->ComputeBindRootCBV(0, _info);

				UINT _x = (_info.vertexCount + 63) / 64;
				a_pCtx->Dispatch(_x, 1, 1);
			}
	}
}
