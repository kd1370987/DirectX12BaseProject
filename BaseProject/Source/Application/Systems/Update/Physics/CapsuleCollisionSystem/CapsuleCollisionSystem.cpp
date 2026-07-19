#include "CapsuleCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/CapsuleCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

namespace
{
	// カプセルをHLSLのカプセル形状（DrawCapsule）で描画する。
	//
	// ベースメッシュ（GetCapsulePoint）は次の単位カプセル :
	//   ・半径          r = 0.5
	//   ・円柱の半長     h = 0.5 （＝上下の球中心が y = ±0.5）
	// これを 半径 a_radius・球中心間の距離 a_height に合わせてスケールする。
	//   ・XZ … 半径を合わせる      : 0.5 -> a_radius        => scale = a_radius * 2
	//   ・Y  … 球中心間を合わせる  : 1.0(=±0.5) -> a_height => scale = a_height
	// ※ ベースが「半球半径 == 円柱半長」固定比のため、a_height != a_radius*2 のときは
	//    上下のキャップが楕円に伸びる（当たり判定の線分自体は常に一致）。
	void DrawCapsuleUpright(
		const DXSM::Vector3& a_center,
		float a_radius,
		float a_height,
		const DXSM::Color& a_color)
	{
		DXSM::Matrix _mat =
			DXSM::Matrix::CreateScale(a_radius * 2.0f, a_height, a_radius * 2.0f) *
			DXSM::Matrix::CreateTranslation(a_center);

		Engine::Editor::MainEditor::Instance().DrawCapsule(_mat, a_color);
	}
}

void CapsuleCollisionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const CapsuleColliderComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"CapsuleCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_activeTag,
			const CapsuleColliderComponent* a_capArray,
			const LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const CapsuleColliderComponent& _cap = a_capArray[_i];
				const LocalTransformComponent& _trans = a_transArray[_i];

				// 中心（ワールドY軸方向の直立カプセル）
				DXSM::Vector3 _center = DXSM::Vector3(_trans.pos) + DXSM::Vector3(_cap.offset);
				DXSM::Vector3 _half = { 0.0f, _cap.height * 0.5f, 0.0f };
				DXSM::Vector3 _pointA = _center - _half;	// 下端の球中心
				DXSM::Vector3 _pointB = _center + _half;	// 上端の球中心

				// カプセル判定情報を作成
				Engine::Collision::CapsuleInfo _info;
				_info.pointA = _pointA;
				_info.pointB = _pointB;
				_info.radius = _cap.radius;

				// コリジョンワールドへ問い合わせ（自分自身は除外）
				Engine::Collision::Result _res = {};
				bool _isHit = _pCollWorld->VsCapsule(_info, _res, a_pChunk->entityData[_i]);

				// デバッグ描画（当たっていれば赤、なければ緑）
				// カプセルは直立（ワールドY軸方向）。中心・半径・球中心間距離(height)で描画。
				DrawCapsuleUpright(
					_center, _cap.radius, _cap.height,
					_isHit ? Engine::Color::RED : Engine::Color::GREEN);

				// 接触点を青い球で表示
				if (_isHit)
				{
					DirectX::BoundingSphere _hitSphere;
					_hitSphere.Center = _res.hitPos;
					_hitSphere.Radius = 0.1f;
					Engine::Editor::MainEditor::Instance().DrawSphere(_hitSphere, Engine::Color::BLUE);
				}
			}
		}
	);
}
