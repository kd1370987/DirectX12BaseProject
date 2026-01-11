#include "CalcMatrixSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../Components/Transform/TRSComponent.h"
#include "../../../Components/Transform/WorldMatrixComponent.h"

void CalcMatrixSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<TRSComponent, WorldMatrixComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			TRSComponent* a_trsArray,
			WorldMatrixComponent* a_worldMatArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				TRSComponent& _trsComp = a_trsArray[_i];
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
