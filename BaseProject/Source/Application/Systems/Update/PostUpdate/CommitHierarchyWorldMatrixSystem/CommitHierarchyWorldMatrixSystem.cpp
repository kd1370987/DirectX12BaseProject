#include "CommitHierarchyWorldMatrixSystem.h"
#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"

#include "../../../../InstanceResource/HierarchyResource.h"

void CommitHierarchyWorldMatrixSystem::Init(Engine::ECS::World& a_world)
{
	// ヒエラルキーがついていない単体オブジェクトに対して最終行列を作成する
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<LocalTransformComponent,HierarchyComponent>(),
		Engine::ECS::WriteList<WorldMatrixComponent>(),
		[&a_world]()
		{
			auto& _hRes = a_world.GetResource<HierarchyResource>();
			for (int _depth = 0; _depth <= _hRes.maxDepth; ++_depth)
			{
				a_world.ForEach<LocalTransformComponent, WorldMatrixComponent, HierarchyComponent>(
					[_depth,&a_world]
					(
						Engine::ECS::ArchetypeChunk* a_pChunk,
						uint32_t a_count,
						float a_dt,
						LocalTransformComponent* a_trsArray,
						WorldMatrixComponent* a_worldMatArray,
						HierarchyComponent* a_hArray
						)
					{

						for (size_t _i = 0; _i < a_count; ++_i)
						{
							const HierarchyComponent& _hComp = a_hArray[_i];
							if (_hComp.depth != _depth) continue;	// 深度値チェック

							const LocalTransformComponent& _trsComp = a_trsArray[_i];
							if (!_trsComp.isDirty) continue;	// 変更がなければ更新しない

							WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
							
							// 変換行列計算
							DirectX::XMMATRIX _transMat = DirectX::XMMatrixTranslationFromVector(
								DirectX::XMLoadFloat3(&_trsComp.pos)
							);
							DirectX::XMVECTOR _quat = DirectX::XMQuaternionNormalize(
								DirectX::XMLoadFloat4(&_trsComp.quat)
							);
							DirectX::XMMATRIX _rotMat = DirectX::XMMatrixRotationQuaternion(
								_quat
							);
							DirectX::XMMATRIX _scaleMat = DirectX::XMMatrixScalingFromVector(
								DirectX::XMLoadFloat3(&_trsComp.scale)
							);

							DirectX::XMMATRIX _myLocalMat = _scaleMat * _rotMat * _transMat;

							// 2親の行列を掛け合わせる
							if (_hComp.parentID != Engine::ECS::Limits::INVALID_ENTITY) {
								// 親のワールド行列を取得
								auto* _parentMatComp = a_world.RefData<WorldMatrixComponent>(_hComp.parentID);
								DirectX::XMMATRIX _parentMat = DirectX::XMLoadFloat4x4(&_parentMatComp->worldMat);

								// 親 * 子 の順で掛け合わせる
								DirectX::XMMATRIX _worldMat = _myLocalMat * _parentMat;
								DirectX::XMStoreFloat4x4(&_worldMatComp.worldMat, _worldMat);
							}
							else {
								// 親がいない場合はそのまま（ルート）
								DirectX::XMStoreFloat4x4(&_worldMatComp.worldMat, _myLocalMat);
							}

							_trsComp.isDirty = false;
						}

					}
				);
			}
		}
	);
}
