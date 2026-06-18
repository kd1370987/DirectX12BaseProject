#include "CalcTransformFromExoskeletonSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Hierarchy/ExoskeletonAttachementComponent.h"
#include "../../../../Components/Resource/AnimatorComponent.h"
#include "../../../../Components/Resource/NodePoseComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"

#include "Application/Components/Transform/TransformComponent.h"

void CalccTransformFromExoskeletonSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ExoskeletonAttachmentComponent, TransformComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"CalccTransformFromExoskeletonSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ExoskeletonAttachmentComponent* a_exoArray,
			TransformComponent* a_transArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ExoskeletonAttachmentComponent& _exoComp = a_exoArray[_i];
				TransformComponent& _trsComp = a_transArray[_i];

				// 親のトランスフォームを取得
				auto* _pParentTrans = a_world.RefData<TransformComponent>(_exoComp.parentID);
				if (!_pParentTrans) continue;

				// 親のノード配列を取得
				auto* _pNodePoseComp = a_world.RefData<NodePoseComponent>(_exoComp.parentID);
				if (!_pNodePoseComp) continue;

				// ノード行列配列取得
				auto& _nodePosePool = a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				auto _nodePoseVec = _nodePosePool.RefRange(_pNodePoseComp->nodePoseHandle);
				if (_nodePoseVec.empty()) continue;

				// 対象の行列を取得
				const DirectX::XMMATRIX& _nodeMat = DirectX::XMLoadFloat4x4(&_nodePoseVec[_exoComp.targetNodeIdx].world);

				// 親Entity自体のワールド行列を作成
				DirectX::XMMATRIX _parentWorldMat = 
					DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&_pParentTrans->quat))
					* DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&_pParentTrans->pos));
				DirectX::XMMATRIX _boneWorldMat = _nodeMat * _parentWorldMat;

				// 外骨格のオフセット行列を作成
				DirectX::XMMATRIX _offsetMat = 
					DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&_exoComp.offsetScale)) * 
					DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&_exoComp.offsetRotation)) *
					DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&_exoComp.offsetPosition));
				// 最終的な外骨格のワールド行列
				DirectX::XMMATRIX _finalWorldMat = _offsetMat * _boneWorldMat;

				// 行列から最終的な 位置と回転を抽出して書き戻す
				DirectX::XMVECTOR _outScale;
				DirectX::XMVECTOR _outQuat;
				DirectX::XMVECTOR _outPos;
				DirectX::XMMatrixDecompose(&_outScale, &_outQuat, &_outPos, _finalWorldMat);

				DirectX::XMStoreFloat3(&_trsComp.pos, _outPos);
				DirectX::XMStoreFloat4(&_trsComp.quat, _outQuat);
				DirectX::XMStoreFloat3(&_trsComp.scale, _outScale);
			}
		}
	);
}
