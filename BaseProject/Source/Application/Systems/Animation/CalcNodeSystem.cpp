#include "CalcNodeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace App::System
{
	void CalcNodeSystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクとプールの自分の範囲だけを書くので、チャンクを分けてワーカーで回す
		a_world.ActiveJobTask<const Component::ModelComponent,const Component::AnimatorComponent, Component::NodePoseComponent>(
			Engine::ECS::ESystemType::Animation,
			"CalcNodeSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx, 
				Component::ActiveTag* /*a_tags*/,
				const Component::ModelComponent* a_modelArray,
				const Component::AnimatorComponent* /*a_animatorArray*/,
				Component::NodePoseComponent* a_nodePoseArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::ModelComponent& _modelComp = a_modelArray[_i];
					Component::NodePoseComponent& _nodeComp = a_nodePoseArray[_i];

					// モデル取得
					auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
					if (!_pModel) continue;

					// ノードポーズ行列配列取得
					auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
					auto _nodePoseVec = _nodePosePool.RefRange(_nodeComp.nodePoseHandle);
					if (_nodePoseVec.empty()) continue;

					// ノードポーズのワールド行列を求める
					for (int _rootIdx : _pModel->GetRootNodeVec())
					{
						Engine::Graphics::Animation::CalcNodeMatrix(
							_rootIdx,
							-1,
							_pModel,
							_nodePoseVec
						);
					}
				}
			}
		)
		// 順序 : クリップと加算ポーズでローカルを組み終えてから、ワールドを組む
		.After("AdditivePoseSystem");
	}
}