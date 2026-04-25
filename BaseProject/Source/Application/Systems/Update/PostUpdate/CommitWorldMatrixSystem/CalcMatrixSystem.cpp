#include "CalcMatrixSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/TransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

void CalcMatrixSystem::Init(Engine::ECS::World& a_world)
{
	a_world.RegisterTask<const TransformComponent, WorldMatrixComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			const TransformComponent* a_trsArray,
			WorldMatrixComponent* a_worldMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TransformComponent& _trsComp = a_trsArray[_i];
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
			}
		}
	);
}