#include "UpdateBLASPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Option/OptionManager.h"

#include "../../../../Scene/SceneManager/SceneManager.h"

#include "../../../../ECS/World/World.h"

namespace Engine::Graphics
{
	void Engine::Graphics::AddUpdateBLASPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "AddUpdateBLASPass";
		_node.phase = a_phase;
		RGComputePassBuilder _rpBuilder(&_node);

		// 実行関数
		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				auto* _pCmdList = a_pCtx->GetCurrentCmdList();
				auto* _pCurrentWorld = Scene::SceneManager::Instance().RefWorld();
				if (!_pCurrentWorld) return;

				// スキニングの書き込み完了を待つバリア
				a_pGE->RefRWAnimatedBuffer().Barrier(_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				// スキニング対象のBLASを更新
				for (auto& _item : a_pGE->GetSkinningImtes())
				{
					auto& _pool = _pCurrentWorld->GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();
					auto* _animMeshData = _pool.Ref(_item.animHandle);
					if (!_animMeshData) continue;

					// メッシュごとにBLASを更新
					for (auto& _animMesh : _animMeshData->meshDataVec)
					{
						_animMesh.instanceBLAS.Update(_pCmdList);
						_animMesh.instanceBLAS.UAVBarrier(_pCmdList);
					}
				}
			};

		// パス登録
		_node.phase = a_phase;
		a_pRegistry->RegisterPass(_node);
	}
}