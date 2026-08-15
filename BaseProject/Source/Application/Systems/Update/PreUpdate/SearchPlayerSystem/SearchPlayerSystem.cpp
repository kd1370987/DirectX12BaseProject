#include "SearchPlayerSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Tag/PlayerControllTag.h"

#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

//==============================================================================
// SearchPlayerSystem
//
// 敵とターゲット(プレイヤー)の距離だけを見て索敵し、TargetEntityComponent へ
// 結果(isFind / isInAttackRange / distance)を書き込む。
//
// ・視界コーン + 壁越し判定(LOS)は廃止した。
//     ボス型の敵(オメガフェニックス方式)は「向き」や「遮蔽」で気づく/見失うのではなく、
//     プレイヤーが一定距離まで近づいたら戦闘に入り、離れれば解除される。
//     背後から近づいても、柱を挟んでも戦闘は始まる。
//
// ・距離は 2 段階。発見したら追従し、攻撃圏まで詰めてから撃つ。
//     detectDistance 以内 … 戦闘モード(isFind)。追従を始める
//     attackDistance 以内 … 攻撃可能(isInAttackRange)。ここで初めて撃つ
//
// ・どちらも「入る距離 / 抜ける距離」のヒステリシス付き。
//   入りと抜けを同じ距離にすると境界上で往復するので、抜ける距離は必ず
//   入る距離以上として扱う(エディターで逆に入力されても max で吸収する)。
//
// ・distance は戦闘モードでなくても実距離を書く(3D距離・高低差込み)。
//   FSM の TargetDistance がそのまま素直な値になる。
//
// ・PreUpdate フェーズに置く理由
//     WorldMatrix は前フレームの PostUpdate で確定済みなので、ここでは 1 フレーム前の
//     姿勢で判定する。プレイヤー入力→Intent と同じ PreUpdate 帯なので、この結果を
//     後段のステートマシン用パラメータへ渡す流れに素直に乗る。
//==============================================================================
namespace
{
	// 攻撃可能距離の可視化色(黄)。Engine::Color には無いのでここで作る
	constexpr DirectX::XMFLOAT4 kAttackRangeColor = { 1.0f, 0.85f, 0.1f, 1.0f };

	// 索敵範囲を水平の円でデバッグ描画する
	void DrawRangeCircle(
		Engine::Editor::MainEditor* a_pEditor,
		const DXSM::Vector3&        a_center,	// 円の中心(敵の位置)
		float                       a_radius,	// 半径
		const DXSM::Color&          a_color)
	{
		if (!a_pEditor)         return;
		if (a_radius <= 1e-4f)  return;

		constexpr int _kSeg = 32;	// 円周の分割数

		DXSM::Vector3 _prev = {};
		for (int _s = 0; _s <= _kSeg; ++_s)
		{
			float _t = (DirectX::XM_2PI * _s) / _kSeg;
			DXSM::Vector3 _p = a_center + DXSM::Vector3(std::sin(_t), 0.0f, std::cos(_t)) * a_radius;

			if (_s > 0) a_pEditor->DrawLine(_prev, _p, a_color);
			_prev = _p;
		}
	}
}

void SearchPlayerSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, TargetEntityComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"SearchPlayerSystem",
		[](
			Engine::ECS::ArchetypeChunk*     a_pChunk,
			uint32_t                         a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                       a_tags,
			const WorldMatrixComponent*      a_worldMatArray,
			TargetEntityComponent*           a_targetEntityArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _worldComp = a_worldMatArray[_i];
				TargetEntityComponent&      _target    = a_targetEntityArray[_i];

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
				// ターゲット位置の取得
				//==================================================
				DXSM::Vector3 _selfPos   = DXSM::Matrix(_worldComp.worldMat).Translation();
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
				// 距離判定(2段階・それぞれヒステリシス付き)
				//   発見    : 未発見 → 発見 は detectDistance 以内、
				//             発見 → 未発見 は detectExitDistance より遠く
				//   攻撃可能: 同じ形で attackDistance / attackExitDistance
				//
				// 攻撃可能は戦闘モードの内側でしか成立させない。発見していない相手を
				// 距離だけで撃ち始めないようにするため。
				//==================================================
				if (!_hasTarget)
				{
					// 対象がいない間は戦闘に入らない
					_target.isFind          = false;
					_target.isInAttackRange = false;
					_target.distance        = 0.0f;
					continue;
				}

				float _dist = (_playerPos - _selfPos).Length();

				const float _detect      = _target.detectDistance;
				const float _detectExit  = (std::max)(_target.detectExitDistance, _detect);
				const float _attack      = _target.attackDistance;
				const float _attackExit  = (std::max)(_target.attackExitDistance, _attack);

				// 直前の状態によって見る距離を切り替える(入る距離 / 抜ける距離)
				_target.isFind = _target.isFind
					? (_dist <= _detectExit)
					: (_dist <= _detect);

				_target.isInAttackRange = _target.isFind && (_target.isInAttackRange
					? (_dist <= _attackExit)
					: (_dist <= _attack));

				_target.distance = _dist;

				//==================================================
				// 可視化
				//   発見距離     : 赤=戦闘中 / 緑=非戦闘
				//   解除距離     : 青(戦闘中のみ)
				//   攻撃可能距離 : 黄(戦闘中のみ。圏内は破線ではなく色で判別できないので
				//                  IsInAttackRange はインスペクタで見る)
				//==================================================
				auto* _pEditor = a_ctx.pServices->pMainEditor;
				DrawRangeCircle(_pEditor, _selfPos, _detect,
					_target.isFind ? Engine::Color::RED : Engine::Color::GREEN);
				if (_target.isFind)
				{
					DrawRangeCircle(_pEditor, _selfPos, _detectExit, Engine::Color::BLUE);
					DrawRangeCircle(_pEditor, _selfPos, _attack, kAttackRangeColor);
				}
			}
		}
	);
}
