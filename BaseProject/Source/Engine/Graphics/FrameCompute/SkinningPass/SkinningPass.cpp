#include "SkinningPass.h"

#include "Engine/Graphics/GraphicsEngine.h"

#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Graphics/PipelineState/PipelineStateManager/PipelineStateManager.h"


#include "Engine/Graphics/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"
#include "Engine/Graphics/Frame/MeshBufferAllocator/MeshBufferAllocator.h"
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
		// PSOはハンドルで持つ : 8bitの添字へ落とすと256個目から別のPSOを引く
		Engine::Handle<ID3D12PipelineState> psoHandle = {};
		Engine::Graphics::PipelineStateManager* pPSOManager = nullptr;
	};
	SkinningRuntime g_skinning = {};
}

void Engine::Graphics::SetupSkinning(PipelineStateManager* a_pPSOManager, Resource::ResourceManager& a_resourceManager)
{
	if (!a_pPSOManager) return;
	g_skinning.pPSOManager = a_pPSOManager;

	// シェーダーからルートシグネチャとコンピュートPSOを起こす
	auto _csHandle = Resource::ShaderIO::Request(a_resourceManager, "Asset/Shader/Source/Geometry/Skinning/Skinning.cso");
	auto* _pShader = a_resourceManager.Ref(_csHandle);
	if (!_pShader || !_pShader->Get()) return;

	g_skinning.rootSigHandle = a_pPSOManager->Request(_pShader->Get());

	D3D12::ComputePipelineDesc _desc = {};
	_desc.SetName("SkinningCS");
	_desc.desc.CS.pShaderBytecode = _pShader->Get()->GetBufferPointer();
	_desc.desc.CS.BytecodeLength = _pShader->Get()->GetBufferSize();
	_desc.SetRootSignature(a_pPSOManager->GetRootSignature(g_skinning.rootSigHandle));

	g_skinning.psoHandle = a_pPSOManager->RequestHandle(_desc);
}

// 中身は旧 SkinningPass の実行関数をそのまま移したもの
void Engine::Graphics::ExecuteSkinning(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
{
	if (!a_pGE || !a_pCtx) return;
	if (!g_skinning.pPSOManager) return;

	auto* _spPassData = &g_skinning;

	// このフレームに積まれたスキニング命令
	const auto& _skinningItems = a_pGE->GetDrawLists()->GetSkinningItems();
	{
			auto* _pCmdList = a_pCtx->GetCurrentCmdList();
			auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->psoHandle);

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
				for (auto& _item : _skinningItems)
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

			a_pCtx->BindBindlessHeaps();
			a_pCtx->SetComputeRootSignature(_spPassData->rootSigHandle);
			a_pCtx->SetComputePSO(_pPso);

			// バッファバリア (main を UAV へ : COPY_SOURCE から遷移)
			_pMA->RefAnimatedVertexBuffer().Barrier(_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			// メッシュ情報バインド
			a_pCtx->ComputeBindBonePaletteBuffer(1);
			// バインドレス : 各バッファの番号をルート定数で渡す
			const UINT _vertexIndex   = _pMA->GetStaticVertexBuffer().GetSRV().GetIndex();
			const UINT _indexIndex    = _pMA->GetIndexBuffer().GetSRV().GetIndex();
			const UINT _animatedIndex = _pMA->GetAnimatedVertexBuffer().GetUAV().GetIndex();
			a_pCtx->ComputeBindDescriptorIndices(2, std::span<const UINT>(&_vertexIndex, 1));
			a_pCtx->ComputeBindDescriptorIndices(3, std::span<const UINT>(&_indexIndex, 1));
			a_pCtx->ComputeBindDescriptorIndices(4, std::span<const UINT>(&_animatedIndex, 1));

			for (auto& _item : _skinningItems)
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
				// プールの添字ではなくボーンパレット(GPU)上の位置。
				// ワールドごとの土台が足してあるので、シーンを重ねても他人のボーンを踏まない
				_info.boneOffset = _item.boneBufferStart;
				a_pCtx->ComputeBindRootCBV(0, _info);

				UINT _x = (_info.vertexCount + 63) / 64;
				a_pCtx->Dispatch(_x, 1, 1);
			}
	}
}
