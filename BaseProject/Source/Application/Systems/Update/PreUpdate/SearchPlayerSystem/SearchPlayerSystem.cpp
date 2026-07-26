#include "SearchPlayerSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Collision/ConeCollider.h"
#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Tag/PlayerControllTag.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

//==============================================================================
// SearchPlayerSystem
//
// 敵の前方に「視界コーン」を張り、その内側にターゲット(プレイヤー)が入っているかを
// 判定して TargetEntityComponent に結果(isFind / distance)を書き込む。
// コーンはデバッグ描画する(内側なら赤、そうでなければ緑)。
//
// ・PreUpdate フェーズに置く理由
//     WorldMatrix は前フレームの PostUpdate で確定済みなので、ここでは 1 フレーム前の
//     姿勢で判定する。プレイヤー入力→Intent と同じ PreUpdate 帯なので、この結果を
//     後段のステートマシン用パラメータへ渡す流れに素直に乗る。
//
// ・コーンの形状(ConeColliderComponent)
//     頂点  = 敵の位置 + offset(ローカル空間のオフセットを world 変換した点)
//     軸    = 敵の前方(このエンジンは左手系でローカル +Z が前方)
//     height= 軸方向の長さ(＝探索距離)
//     radius= 最遠面(底面)の半径。軸方向へ進むほど線形に広がる。
//==============================================================================
namespace
{
	// 視界コーンをワイヤーフレームでデバッグ描画する
	void DrawVisionCone(
		Engine::Editor::MainEditor* a_pEditor,
		const DXSM::Vector3&        a_apex,		// 頂点(敵の目の位置)
		const DXSM::Vector3&        a_dir,		// 軸方向(正規化済み)
		float                       a_height,	// 軸方向の長さ
		float                       a_radius,	// 底面の半径
		const DXSM::Color&          a_color)
	{
		if (!a_pEditor)         return;
		if (a_height <= 1e-4f)  return;

		// 軸に直交する基底(_u, _w)を作る。軸がほぼ真上/真下のときは参照ベクトルを変える
		DXSM::Vector3 _ref = (std::fabs(a_dir.y) > 0.99f)
			? DXSM::Vector3(1.0f, 0.0f, 0.0f)
			: DXSM::Vector3(0.0f, 1.0f, 0.0f);
		DXSM::Vector3 _u = a_dir.Cross(_ref);
		_u.Normalize();
		DXSM::Vector3 _w = a_dir.Cross(_u);
		_w.Normalize();

		// 底面(最遠面)の中心
		DXSM::Vector3 _center = a_apex + a_dir * a_height;

		constexpr int _kSeg = 24;	// 円周の分割数
		constexpr int _kRib = 8;	// 頂点から底面へ伸ばす稜線の本数

		DXSM::Vector3 _prev = {};
		for (int _s = 0; _s <= _kSeg; ++_s)
		{
			float _t = (DirectX::XM_2PI * _s) / _kSeg;
			DXSM::Vector3 _p = _center + (_u * std::cos(_t) + _w * std::sin(_t)) * a_radius;

			// 円周を線分でつなぐ
			if (_s > 0) a_pEditor->DrawLine(_prev, _p, a_color);
			_prev = _p;

			// 稜線(頂点 → 底面の円周)
			if ((_s % (_kSeg / _kRib)) == 0) a_pEditor->DrawLine(a_apex, _p, a_color);
		}

		// 中心軸
		a_pEditor->DrawLine(a_apex, _center, a_color);
	}
}

void SearchPlayerSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, const ConeColliderComponent, TargetEntityComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"SearchPlayerSystem",
		[](
			Engine::ECS::ArchetypeChunk*     a_pChunk,
			uint32_t                         a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                       a_tags,
			const WorldMatrixComponent*      a_worldMatArray,
			const ConeColliderComponent*     a_coneColliderArray,
			TargetEntityComponent*           a_targetEntityArray
		)
		{
			// 壁越し判定(LOS)に使う衝突ワールド
			auto* _pCollWorld = a_ctx.pServices->pMainEngine->RefCollisionWorld();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent&  _worldComp = a_worldMatArray[_i];
				const ConeColliderComponent& _cone      = a_coneColliderArray[_i];
				TargetEntityComponent&       _target    = a_targetEntityArray[_i];

				// 今フレームの結果をリセット
				_target.isFind   = false;
				_target.distance = 0.0f;

				//==================================================
				// 探索対象(プレイヤー)の解決
				//   1) 既に Entity が確定していればそれを使う
				//   2) GUID 指定があれば GUID から解決してキャッシュ
				//   3) どちらも無ければ PlayerControllTag から自動で 1 体拾う
				//==================================================
				Engine::ECS::Entity _playerEntity = _target.targetEntity;

				if (_playerEntity == Engine::ECS::Limits::INVALID_ENTITY)
				{
					if (_target.targetGUID != Engine::DefaultGUID)
					{
						_playerEntity = a_ctx.pWorld->GetEntity(_target.targetGUID);
					}
					else
					{
						a_ctx.pWorld->ForEach<const PlayerControllTag>(
							[&_playerEntity](
								Engine::ECS::ArchetypeChunk* a_chunk,
								uint32_t                     a_cnt,
								const PlayerControllTag*)
							{
								if (a_cnt > 0 && _playerEntity == Engine::ECS::Limits::INVALID_ENTITY)
								{
									_playerEntity = a_chunk->entityData[0];
								}
							});
					}

					// 見つかった Entity をキャッシュ(以降は GetEntity/ForEach を回さない)
					_target.targetEntity = _playerEntity;
				}

				//==================================================
				// 敵の頂点・前方向
				//   左手系: ローカル +Z が前方(= worldMat の Z 軸)。
				//   SimpleMath の Vector3::Forward は (0,0,-1) で逆を向くので使わない。
				//==================================================
				DXSM::Matrix  _world(_worldComp.worldMat);
				DXSM::Vector3 _apex = DXSM::Vector3::Transform(DXSM::Vector3(_cone.offset), _world);
				DXSM::Vector3 _dir  = DXSM::Vector3::TransformNormal(DXSM::Vector3(0.0f, 0.0f, 1.0f), _world);

				// 前方が潰れている(scale 0 など)場合は判定不能。NaN も弾ける形で書く
				float _dirLenSq = _dir.LengthSquared();
				if (!(_dirLenSq > 1e-8f)) continue;
				_dir /= std::sqrt(_dirLenSq);

				//==================================================
				// ターゲット位置の取得
				//==================================================
				bool          _hasTarget = false;
				DXSM::Vector3 _playerPos = {};
				if (_playerEntity != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_playerEntity))
				{
					if (const auto* _pPlayerWorld = a_ctx.pWorld->RefData<WorldMatrixComponent>(_playerEntity))
					{
						_playerPos = DXSM::Matrix(_pPlayerWorld->worldMat).Translation();
						_hasTarget = true;
					}
				}

				//==================================================
				// コーン内包判定(中身の詰まったコーン)
				//   1) 軸方向の射影距離が [0, height] に収まるか
				//   2) その距離でのコーン半径(radius * axial/height)以内に居るか
				//   3) 頂点→対象へレイを撃ち、壁などに遮られていないか(LOS)
				//
				// 可視化の色: 緑=対象なし/コーン外, 青=コーン内だが遮蔽, 赤=発見
				//==================================================
				DXSM::Color _color = Engine::Color::GREEN;
				if (_hasTarget && _cone.height > 1e-4f)
				{
					DXSM::Vector3 _toTarget = _playerPos - _apex;
					float _axial = _toTarget.Dot(_dir);

					if (_axial >= 0.0f && _axial <= _cone.height)
					{
						// 軸からの垂直距離
						DXSM::Vector3 _perp   = _toTarget - _dir * _axial;
						float         _radial = _perp.Length();

						// その距離におけるコーンの半径(線形に広がる)
						float _coneR = _cone.radius * (_axial / _cone.height);

						if (_radial <= _coneR)
						{
							// コーン内。ここから壁越し(遮蔽)チェック
							_color = Engine::Color::BLUE;	// コーン内・遮蔽の既定色

							float _dist    = _toTarget.Length();
							bool  _visible = true;

							if (_pCollWorld && _dist > 1e-4f)
							{
								Engine::Collision::RayInfo _ray;
								_ray.origin      = _apex;
								_ray.direction   = _toTarget / _dist;	// 対象への単位方向
								_ray.maxDistance = _dist;

								Engine::Collision::Result _res = {};
								// 自分自身は除外して撃つ
								bool _hit = _pCollWorld->Raycast(
									_ray, _res, a_pChunk->entityData[_i]);

								// 対象以外に当たっていれば、間に遮蔽物がある
								if (_hit && _res.hitEntity != _playerEntity) _visible = false;
							}

							if (_visible)
							{
								_target.isFind   = true;
								_target.distance = _dist;
								_color           = Engine::Color::RED;
							}
						}
					}
				}

				// 可視化(赤=発見 / 青=コーン内だが遮蔽 / 緑=それ以外)
				DrawVisionCone(
					a_ctx.pServices->pMainEditor,
					_apex, _dir, _cone.height, _cone.radius, _color);
			}
		}
	);
}
