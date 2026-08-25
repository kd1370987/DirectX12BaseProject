#include "LockOnTargetSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Option/OptionManager.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Tag/EnemyTag.h"
#include "Application/Components/Camera/ProjMatComponent.h"
#include "Application/Components/Character/LockOnTargetComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/InstanceResource/SingletonEntityResource.h"

#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

//==========================================================================================
// LockOnTargetSystem
//
// 画面内に映っている敵を集め、そのうちレティクル(画面中央の円)の内側で
// いちばん中心に近いものをロック対象にする。
//
//   画面内         … LockOnTargetComponent.targets[](HUD が黄色い枠で囲う)
//   レティクル内で最近 … lockedEntity(HUD が赤い枠で囲い、プレイヤーが体を向ける)
//
// ・枠(画面内)とロック(レティクル内)は条件が別。枠が出ている相手すべてが
//   ロック対象になるわけではない。
//   距離(maxDistance)が効くのはロックだけで、枠は距離を見ない。
//   遠くの敵にも枠が出ないと、湧いたウェーブがどこに居るのか画面から分からないため。
//
// ・PostUpdate に置く理由
//     判定はスクリーン座標で行うので、カメラと敵の「今フレームの」ワールド行列が要る。
//     WorldMatrixComponent を読むので、それを書く CalcMatrixSystem /
//     CommitHierarchyWorldMatrixSystem より自動的に後ろへ回る(同じ PostUpdate 帯)。
//     HUD(GameObjectManager::Update)はさらに後なので、枠の位置は描画される絵と一致する。
//     体の向き(LockOnRotationSystem)は次フレームの Update で読むため 1 フレーム遅れるが、
//     旋回は Slerp で追従させるので見た目には出ない。
// ・スクリーン座標もここで作って持たせる。HUD 側で射影をやり直すと、
//   条件のわずかな違いで「枠は出るのにロックされない」といったズレが起きる。
//==========================================================================================
void LockOnTargetSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PlayerControllTag, const WorldMatrixComponent, LockOnTargetComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"LockOnTargetSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const PlayerControllTag* a_playerTagArray,
			const WorldMatrixComponent* a_worldMatArray,
			LockOnTargetComponent* a_lockOnArray
		)
		{
			if (a_count == 0) return;
			if (!a_ctx.pServices || !a_ctx.pServices->pOptionManager) return;

			// スクリーン解像度(px) : HUD の SubmitUI がピクセル座標を解釈する基準と合わせる
			const auto& _winOp = a_ctx.pServices->pOptionManager->GetWindowOption();
			const float _w = static_cast<float>(_winOp.windowWidth);
			const float _h = static_cast<float>(_winOp.windowHeight);
			if (_w <= 0.0f || _h <= 0.0f) return;

			//==================================================================
			// アクティブカメラから ViewProj を作る
			//   ビュー行列はカメラのワールド行列の逆行列
			//==================================================================
			Math::Matrix _viewProjMat = Math::Matrix::Identity();
			bool _hasCamera = false;

			// どのカメラで映しているかは MainCameraSystem が決めて
			// SingletonEntityResource へ置いてあるので、ここでは探さずに引くだけ
			if (a_ctx.pWorld->HasResource<SingletonEntityResource>())
			{
				const Engine::ECS::Entity _camera =
					a_ctx.pWorld->GetResource<SingletonEntityResource>().mainCamera;

				if (_camera != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->IsAliveEntity(_camera) &&
					a_ctx.pWorld->HasComponent<ProjMatComponent>(_camera) &&
					a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_camera))
				{
					const auto* _pProjMat  = a_ctx.pWorld->RefData<ProjMatComponent>(_camera);
					const auto* _pWorldMat = a_ctx.pWorld->RefData<WorldMatrixComponent>(_camera);

					if (_pProjMat && _pWorldMat)
					{
						const Math::Matrix _camWorldMat = _pWorldMat->worldMat;

						_viewProjMat = _camWorldMat.Invert() * _pProjMat->projMat;
						_hasCamera   = true;
					}
				}
			}

			const Math::Vector2 _defaultCenter = { _w * 0.5f, _h * 0.5f };

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _playerWorld = a_worldMatArray[_i];
				LockOnTargetComponent&      _lockOn      = a_lockOnArray[_i];

				// このフレームぶんを作り直す
				_lockOn.targetCount  = 0;
				_lockOn.lockedEntity = Engine::ECS::Limits::INVALID_ENTITY;

				// カメラがいないフレームは誰も狙えない
				if (!_hasCamera) continue;

				const Math::Matrix& _pm = _playerWorld.worldMat;
				const Math::Vector3 _playerPos = { _pm._41, _pm._42, _pm._43 };

				// 判定の円。内側のレティクル(AimReticleHUD)が居ればそれに合わせる。
				// UI が無い構成でも動くよう、届いていなければ画面中央 × 設定値で判定する
				const Math::Vector2 _reticleCenter = _lockOn.isReticleFromHUD
					? Math::Vector2(_lockOn.reticleCenter)
					: _defaultCenter;
				const float _reticleRadius = _lockOn.GetActiveReticleRadius();

				// 画面中央にいちばん近いものを選ぶための最小値
				float _nearestDist = FLT_MAX;

				a_ctx.pWorld->ForEach<const ActiveTag, const EnemyTag, const WorldMatrixComponent>(
					[&](
						Engine::ECS::ArchetypeChunk* a_pEnemyChunk,
						uint32_t a_enemyCount,
						const ActiveTag* a_activeTagArray,
						const EnemyTag* a_enemyTagArray,
						const WorldMatrixComponent* a_enemyWorldMatArray
					)
					{
						for (uint32_t _e = 0; _e < a_enemyCount; ++_e)
						{
							// ワールド行列の平行移動成分がその敵の位置
							const Math::Matrix& _em = a_enemyWorldMatArray[_e].worldMat;
							Math::Vector3 _worldPos = { _em._41, _em._42, _em._43 };
							_worldPos.y += _lockOn.targetOffsetY;

							//------------------------------------------------------
							// ワールド → スクリーン
							//------------------------------------------------------
							Math::Vector4 _clipPos = Math::Vector4::Transform(_worldPos, _viewProjMat);

							// w <= 0 はカメラ後方。ここで割ると画面内へ折り返して映るので必ず弾く
							if (_clipPos.w <= 1e-4f) continue;

							const float _invW = 1.0f / _clipPos.w;
							const Math::Vector3 _ndc = {
								_clipPos.x * _invW, _clipPos.y * _invW, _clipPos.z * _invW };

							// ニア/ファーの外は狙わない(DirectXの深度は0..1)
							if (_ndc.z < 0.0f || _ndc.z > 1.0f) continue;

							// NDC(中心原点/Y上向き) → ピクセル(左上原点/Y下向き)
							const Math::Vector2 _screenPos = {
								(_ndc.x * 0.5f + 0.5f) * _w,
								(0.5f - _ndc.y * 0.5f) * _h
							};

							//------------------------------------------------------
							// 枠 : 画面内に映っているか
							//------------------------------------------------------
							// NDC のままで見る(ピクセルに直してから比べるのと同じ)。
							// 画面外の敵に枠を出すと、端に貼り付いた枠が残ってしまう
							if (_ndc.x < -1.0f || _ndc.x > 1.0f) continue;
							if (_ndc.y < -1.0f || _ndc.y > 1.0f) continue;

							// 枠を出す相手として登録(上限を超えたぶんは表示だけ諦める)
							if (_lockOn.targetCount < LockOnTargetComponent::TARGET_MAX)
							{
								const int _index = _lockOn.targetCount;
								_lockOn.targets[_index]   = a_pEnemyChunk->entityData[_e];
								_lockOn.screenPos[_index] = _screenPos;
								++_lockOn.targetCount;
							}

							//------------------------------------------------------
							// ロック : レティクルの内側で、いちばん中心に近いもの
							//------------------------------------------------------
							// 枠(画面内)より狭い条件。枠が出ている相手すべてが
							// ロック対象になるわけではない
							//
							// 距離で足切り(0 以下なら距離では切らない)。
							// 効くのはロックだけ。枠(上)は距離を見ない。
							// 遠くの敵にも枠が出ないと、湧いたウェーブがどこに居るのか
							// 画面から分からないため。ロックのほうを外すと、
							// 届きもしない距離の相手に体を向けて撃ち始めてしまう
							if (_lockOn.maxDistance > 0.0f &&
								(_worldPos - _playerPos).Length() > _lockOn.maxDistance) continue;

							const float _distFromCenter = (_screenPos - _reticleCenter).Length();
							if (_distFromCenter > _reticleRadius) continue;

							if (_distFromCenter < _nearestDist)
							{
								_nearestDist            = _distFromCenter;
								_lockOn.lockedEntity    = a_pEnemyChunk->entityData[_e];
								_lockOn.lockedScreenPos = _screenPos;
								_lockOn.lockedPos       = _worldPos;
							}
						}
					}
				);

				//==============================================================
				// デバッグ表示 : 向きのズレを目で確かめるための2本
				//--------------------------------------------------------------
				//   緑 … 自機からロック相手への「正しい向き」(このシステムが出す答え)
				//   赤 … 自機が実際に向いている方向(ワールド行列の +Z 軸)
				// 体の旋回が正しければ、追従の遅れぶんを除いてこの2本は重なる。
				// 重なっているのにモデルが横を向いて見えるなら、原因はモデル側
				// (メッシュの正面が +Z からずれている)。
				//==============================================================
				if (_lockOn.IsLocked() && a_ctx.pServices->pMainEditor)
				{
					auto* _pEditor = a_ctx.pServices->pMainEditor;

					Math::Vector3 _toTarget = Math::Vector3(_lockOn.lockedPos) - _playerPos;
					_toTarget.y = 0.0f;
					if (_toTarget.LengthSquared() > 1e-6f)
					{
						_toTarget.Normalize();
						_pEditor->DrawLine(_playerPos, _playerPos + _toTarget * 10.0f, Engine::Color::GREEN);
					}

					// 自機の前方(左手系 +Z = ワールド行列の第3行)
					Math::Vector3 _forward = { _pm._31, _pm._32, _pm._33 };
					_forward.y = 0.0f;
					if (_forward.LengthSquared() > 1e-6f)
					{
						_forward.Normalize();
						_pEditor->DrawLine(_playerPos, _playerPos + _forward * 10.0f, Engine::Color::RED);
					}
				}
			}
		}
	);
}
