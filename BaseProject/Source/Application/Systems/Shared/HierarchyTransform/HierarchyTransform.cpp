#include "HierarchyTransform.h"

#include "../../../Components/Transform/LocalTransformComponent.h"
#include "../../../Components/Hierarchy/HierarchyComponent.h"

namespace App::Systems::HierarchyTransform
{
	namespace
	{
		// 辿る深さの上限。
		// ヒエラルキーが輪になっていた場合の保険で、実運用の階層はせいぜい数段
		constexpr int kMaxDepth = 32;

		//------------------------------------------------------------------------------
		// 親のワールド行列から、継承フラグで指定された成分だけを取り出す
		//
		// CommitHierarchyWorldMatrixSystem と同じ順(S → R → T)で組み直すこと。
		// 順番を変えると部分継承のオブジェクトだけ絵と当たり判定がずれる
		//------------------------------------------------------------------------------
		Math::Matrix FilterParentMatrix(
			const Math::Matrix& a_parentMat,
			ETransformInheritance a_inheritance)
		{
			Math::Matrix _filtered = Math::Matrix::Identity();

			Math::Vector3 _pos = {};
			Math::Quaternion _quat = {};
			Math::Vector3 _scale = {};
			a_parentMat.Decompose(_scale, _quat, _pos);

			if (Engine::Utility::HasFlag(a_inheritance, ETransformInheritance::Scale))
			{
				_filtered *= Math::Matrix::CreateScale(_scale);
			}
			if (Engine::Utility::HasFlag(a_inheritance, ETransformInheritance::Rotation))
			{
				_filtered *= Math::Matrix::CreateFromQuaternion(_quat);
			}
			if (Engine::Utility::HasFlag(a_inheritance, ETransformInheritance::Translation))
			{
				_filtered *= Math::Matrix::CreateTranslation(_pos);
			}

			return _filtered;
		}

		//------------------------------------------------------------------------------
		// 本体。親側から順に掛けていく
		//------------------------------------------------------------------------------
		Math::Matrix CalcWorldMatrixInternal(
			Engine::ECS::World& a_world,
			Engine::ECS::Entity a_entity,
			int a_depth)
		{
			if (a_entity == Engine::ECS::Limits::INVALID_ENTITY) return Math::Matrix::Identity();

			// RefData は持っていなくても非nullが返るので、必ず HasComponent で見る
			if (!a_world.HasComponent<LocalTransformComponent>(a_entity)) return Math::Matrix::Identity();
			const auto* _pTrs = a_world.RefData<LocalTransformComponent>(a_entity);
			if (!_pTrs) return Math::Matrix::Identity();

			// 自分のローカル行列
			Math::Matrix _localMat = Math::Matrix::CreateTRS(
				_pTrs->pos,
				_pTrs->quat.Normalized(),
				_pTrs->scale);

			// これ以上辿らない条件。自分のローカルだけ返す
			if (a_depth >= kMaxDepth) return _localMat;
			if (!a_world.HasComponent<HierarchyComponent>(a_entity)) return _localMat;

			const auto* _pHierarchy = a_world.RefData<HierarchyComponent>(a_entity);
			if (!_pHierarchy) return _localMat;
			if (_pHierarchy->parentID == Engine::ECS::Limits::INVALID_ENTITY) return _localMat;

			// 親のワールド行列
			Math::Matrix _parentMat =
				CalcWorldMatrixInternal(a_world, _pHierarchy->parentID, a_depth + 1);

			// 全継承ならそのまま掛ける
			if (_pTrs->inheritance == ETransformInheritance::All)
			{
				return _localMat * _parentMat;
			}

			// 部分継承は成分を選り分けてから掛ける
			return _localMat * FilterParentMatrix(_parentMat, _pTrs->inheritance);
		}
	}

	Math::Matrix CalcWorldMatrix(
		Engine::ECS::World& a_world,
		Engine::ECS::Entity a_entity)
	{
		return CalcWorldMatrixInternal(a_world, a_entity, 0);
	}
}
