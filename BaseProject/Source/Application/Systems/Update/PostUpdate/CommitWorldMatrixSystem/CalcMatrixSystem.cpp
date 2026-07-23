#include "CalcMatrixSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"

void CalcMatrixSystem::Init(Engine::ECS::World& a_world)
{
	// ヒエラルキーがついていない単体オブジェクトに対して最終行列を作成する
	a_world.ActiveTask<const LocalTransformComponent, WorldMatrixComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"CalcMatrixSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const LocalTransformComponent* a_trsArray,
			WorldMatrixComponent* a_worldMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const LocalTransformComponent& _trsComp = a_trsArray[_i];

				// 変更がなければ更新しない
				if (!_trsComp.isDirty) continue;

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

				DirectX::XMMATRIX _worldMat = _scaleMat * _rotMat * _transMat;

				DirectX::XMStoreFloat4x4(&_worldMatComp.worldMat, _worldMat);

				// mutubleでconstを無視している
				_trsComp.isDirty = false;
			}
		},
		Engine::ECS::Exclude<HierarchyComponent>()
	);
}