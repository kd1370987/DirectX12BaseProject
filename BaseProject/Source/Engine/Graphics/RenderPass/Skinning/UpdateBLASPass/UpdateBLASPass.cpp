#include "UpdateBLASPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocator/CBAllocator.h"

#include "Engine/Option/OptionManager.h"

#include "../../../../Scene/SceneManager/SceneManager.h"

#include "../../../../ECS/World/World.h"
#include "../../../../Graphics/MeshBufferAllocator/MeshBufferAllocator.h"

namespace Engine::Graphics
{
	//======================================================================================
	// スキニング結果からBLASを更新する
	//
	// カメラに依存せず、フレームに1回で足りるのでレンダーグラフには載せない。
	// シェーダーもPSOも使わないので用意する関数は無い。
	// 中身は旧 UpdateBLASPass の実行関数をそのまま移したもの
	//======================================================================================
	void ExecuteUpdateBLAS(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		if (!a_pGE || !a_pCtx) return;
		{
				auto* _pCmdList = a_pCtx->GetCurrentCmdList();

				auto* _pMA = a_pGE->RefMeshBufferAllocator();
				if (!_pMA) return;

				// SRVへバッファを遷移
				_pMA->RefAnimatedVertexBuffer().Barrier(
					_pCmdList, 
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);
				// スキニング対象のBLASを更新
				//
				// プールは命令を出したワールドのものを引く。
				// 以前は SceneManager::RefWorld() =「一番上のシーン」から引いていたため、
				// ポーズ画面のようにシーンを重ねている間は後ろのゲームのキャラのハンドルを
				// ポーズ側のプールで探すことになり、動的BLASが更新されなかった
				// (レイトレのGIや影からキャラが居なくなる)。
				for (auto& _item : a_pGE->GetSkinningImtes())
				{
					if (!_item.pWorld) continue;
					if (!_item.pWorld->HasResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>()) continue;

					auto& _pool = _item.pWorld->GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();
					auto* _animMeshData = _pool.Ref(_item.animHandle);
					if (!_animMeshData) continue;

					// メッシュごとにBLASを更新
					for (auto& _animMesh : _animMeshData->meshDataVec)
					{
						_animMesh.instanceBLAS.Update(_pCmdList);
						_animMesh.instanceBLAS.UAVBarrier(_pCmdList);
					}
				}
		}
	}
}