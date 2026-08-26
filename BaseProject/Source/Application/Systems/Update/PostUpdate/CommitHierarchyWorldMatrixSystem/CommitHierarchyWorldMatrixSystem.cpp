#include "CommitHierarchyWorldMatrixSystem.h"
#include "Application/ECS/World/World.h"

#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"

#include "../../../../InstanceResource/HierarchyResource.h"

void CommitHierarchyWorldMatrixSystem::Init(App::ECS::World& a_world)
{
	// ヒエラルキーがついていない単体オブジェクトに対して最終行列を作成する
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<LocalTransformComponent, HierarchyComponent>{},
		Engine::ECS::WriteList<WorldMatrixComponent>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			// 更新前にワールド行列のフラグを消しておく
			a_ctx.pWorld->ForEach<WorldMatrixComponent>(
				[]
				(
					Engine::ECS::ArchetypeChunk* a_pChunk,
					uint32_t a_count,
					WorldMatrixComponent* a_worldMatArray
					)
				{
					for (size_t _i = 0; _i < a_count; ++_i)
					{
						auto& _comp = a_worldMatArray[_i];
						_comp.wasUpdatedThisFrame = false;
					}
				}
			);

			// 深度ごとに親子階層の更新をする
			auto& _hRes = a_ctx.pWorld->GetResource<HierarchyResource>();
			for (int _depth = 0; _depth <= _hRes.maxDepth; ++_depth)
			{
				a_ctx.pWorld->ForEach<LocalTransformComponent, WorldMatrixComponent, HierarchyComponent>(
					[_depth, &a_ctx]
					(
						Engine::ECS::ArchetypeChunk* a_pChunk,
						uint32_t a_count,
						LocalTransformComponent* a_trsArray,
						WorldMatrixComponent* a_worldMatArray,
						HierarchyComponent* a_hArray
						)
					{

						for (size_t _i = 0; _i < a_count; ++_i)
						{
							const HierarchyComponent& _hComp = a_hArray[_i];
							if (_hComp.depth != _depth) continue;	// 深度値チェック

							bool _isParentUpdated = false;
							if (_hComp.parentID != Engine::ECS::Limits::INVALID_ENTITY) {
								if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_hComp.parentID)) continue;
								auto* _parentMatComp = a_ctx.pWorld->RefData<WorldMatrixComponent>(_hComp.parentID);
								if (_parentMatComp) {
									_isParentUpdated = _parentMatComp->wasUpdatedThisFrame;
								}
							}

							const LocalTransformComponent& _trsComp = a_trsArray[_i];
							WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
							if (!_trsComp.isDirty && !_isParentUpdated) {
								_worldMatComp.wasUpdatedThisFrame = false; // 自分も更新なし
								continue;
							}

							// ローカル上で行列を作成
							Math::Matrix _myLocalMat = Math::Matrix::CreateTRS(
								_trsComp.pos,
								_trsComp.quat.Normalized(),
								_trsComp.scale);

							// 親の行列を掛け合わせる
							if (_hComp.parentID != Engine::ECS::Limits::INVALID_ENTITY) {
								// 親のワールド行列を取得
								auto* _parentMatComp = a_ctx.pWorld->RefData<WorldMatrixComponent>(_hComp.parentID);
								if (!_parentMatComp) continue;


								if (_trsComp.inheritance == ETransformInheritance::All)
								{
									// 子 * 親 の順(自分のローカルを先に適用してから親へ乗せる)
									_worldMatComp.worldMat = _myLocalMat * _parentMatComp->worldMat;
								}
								else
								{
									// 分解
									Math::Matrix _parentMat = Math::Matrix::Identity();
									Math::Vector3 _pos = {};
									Math::Quaternion _quat = {};
									Math::Vector3 _scale = {};
									_parentMatComp->worldMat.Decompose(_scale,_quat, _pos);

									if (Engine::Utility::HasFlag(_trsComp.inheritance, ETransformInheritance::Scale))
									{
										_parentMat *= Math::Matrix::CreateScale(_scale);
									}
									if (Engine::Utility::HasFlag(_trsComp.inheritance, ETransformInheritance::Rotation))
									{
										_parentMat *= Math::Matrix::CreateFromQuaternion(_quat);
									}
									if (Engine::Utility::HasFlag(_trsComp.inheritance, ETransformInheritance::Translation))
									{
										_parentMat *= Math::Matrix::CreateTranslation(_pos);
									}
									
									_worldMatComp.worldMat = _myLocalMat * _parentMat;
								}
							}
							else {
								// 親がいない場合はそのまま（ルート）
								_worldMatComp.worldMat = _myLocalMat;
							}

							_worldMatComp.wasUpdatedThisFrame = true;

							_trsComp.isDirty = false;
						}

					}
				);
			}
		}
	);
}
