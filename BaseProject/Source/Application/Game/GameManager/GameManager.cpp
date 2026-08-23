#include "GameManager.h"

// エンジン
#include "../../../Engine/MainEngine.h"

// シーン関係
#include "../../../Engine/Scene/SceneManager/SceneManager.h"

// ECS関係
#include "../../../Engine/ECS/World/World.h"

// ECS外オブジェクト(クラスメタマネージャー / 登録するクラス)
#include "../../../Engine/GameObject/ObjectMetaRegistry/ObjectMetaRegistry.h"
#include "Application/Object/UI/CombatReticleHUD/CombatReticleHUD.h"
#include "Application/Object/UI/TargetBoxHUD/TargetBoxHUD.h"
#include "Application/Object/UI/AimReticleHUD/AimReticleHUD.h"
#include "Application/Object/UI/HitEffectHUD/HitEffectHUD.h"
#include "Application/Object/UI/ScoreHUD/ScoreHUD.h"
#include "Application/Object/Sequence/ResultSequence/ResultSequence.h"
#include "Application/Object/UI/MissileLockBoxHUD/MissileLockBoxHUD.h"
#include "Application/Object/UI/UIButton/UIButton.h"
#include "Application/Object/UI/UIImage/UIImage.h"
#include "Application/Object/Sequence/TitleSequence/TitleSequence.h"
#include "Application/Object/Sequence/HomeSequence/HomeSequence.h"
#include "Application/Object/Sequence/PauseSequence/PauseSequence.h"
#include "Application/Object/Sequence/MissionSelect/MissionSelect.h"
#include "../../Object/Scene/SceneAmbientObject/SceneAmbientObject.h"
#include "Application/Object/Sequence/SceneSequence/SceneSequence.h"

// コンポーネント関係
// システムフェーズタグ
#include "Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "Application/Components/Tag/SystemPhaseTag/AwakeTag.h"
#include "Application/Components/Tag/SystemPhaseTag/StartTag.h"
#include "Application/Components/Tag/SystemPhaseTag/ActiveTag.h"

// コンポーネント
#include "Application/Components/Tag/RenderTag/RayTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"
#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Tag/CameraControllTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"
#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "Application/Components/Camera/TPSLookAngleComponent.h"
#include "Application/Components/Force/GravityComponent.h"
#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/MovementComponent.h"
#include "Application/Components/Character/LookAngleComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Intent/MoveIntentComponent.h"
#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Resource/UIComponent.h"
#include "Application/Components/Resource/StateMachineComponent.h"
#include "Application/Components/Persistence/GUIDComponent.h"
#include "Application/Components/Persistence/NameComponent.h"
#include "Application/Components/Hierarchy/HierarchyComponent.h"
#include "Application/Components/Hierarchy/FollowAnimationNodeComponent.h"
#include "Application/Components/Hierarchy/SpawnerComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Character/Robot/BoostComponent.h"
#include "Application/Components/Character/Robot/BoosterEffectComponent.h"
#include "Application/Components/Character/Robot/ChargeDashComponent.h"
#include "Application/Components/Character/ScoreTargetComponent.h"
#include "Application/Components/Character/Robot/AttachmentSlotsComponent.h"
#include "Application/Components/Resource/ParticlesComponent.h"
#include "Application/Components/Camera/TPSCameraStateComponent.h"
#include "Application/Components/Camera/TPSFollowComponent.h"
#include "Application/Components/Collision/SphereCollider.h"
#include "Application/Components/Collision/BoxCollider.h"
#include "Application/Components/Collision/OBBCollider.h"
#include "../../Components/Tag/EnemyTag.h"
#include "../../Components/Collision/CapsuleCollider.h"
#include "../../Components/Intent/ActionIntentComponent.h"
#include "../../Components/Character/Weapon/Gun/GunStateComponent.h"
#include "../../Components/Character/Weapon/WeaponTriggerComponent.h"
#include "Engine/ECS/Internal/CollisionEvent.h"
#include "../../Components/Collision/ExplodeOnHitComponent.h"
#include "../../Components/Camera/CameraFocusTargetComponent.h"
#include "../../Components/Camera/CameraDeadZoneComponent.h"
#include "../../Components/Character/Robot/AdditivePoseComponent.h"
#include "../../Components/Character/AimTargetPosComponent.h"
#include "../../Components/Character/TargetEntityComponent.h"
#include "../../Components/Character/LockOnTargetComponent.h"
#include "../../Components/Resource/SoundComponent.h"
#include "../../Components/Character/PatrolComponent.h"
#include "../../Components/Character/Weapon/Projectile/HomingComponent.h"
#include "../../Components/Character/Weapon/Projectile/ProjectileComponent.h"
#include "../../Components/Character/Weapon/Missile/MissileLockComponent.h"
#include "../../Components/Character/Boss/BossComponent.h"

// システム関連
#include "Application/Systems/Init/PostDeserialize/ModelFixupSystem/ModelFixupSystem.h"
#include "Application/Systems/Init/PostDeserialize/GUIDFixupSystem/GUIDFixupSystem.h"
#include "Application/Systems/Init/Awake/ModelReadyGateSystem/ModelReadyGateSystem.h"
#include "Application/Systems/Init/Awake/AttachmentReadyGateSystem/AttachmentReadyGateSystem.h"
#include "Application/Systems/Init/Awake/FollowTargetLinkSystem/FollowTargetLinkSystem.h"
#include "Application/Systems/Init/Awake/HierarchyLinkSystem/HierarchyLinkSystem.h"
#include "Application/Systems/Init/Start/CameraStartSystem/CameraStartSystem.h"
#include "Application/Systems/Init/Start/AnimationModelStartSystem/AnimationModelStartSystem.h"
#include "Application/Systems/Init/Start/RegisterCollisionWorldSystem/RegisterCollisionWorldSystem.h"
#include "Application/Systems/Init/Start/AttachmentNodeLinkSystem/AttachmentNodeLinkSystem.h"
#include "Application/Systems/Update/Input/InputMoveSystem/InputMoveSystem.h"
#include "Application/Systems/Update/Update/Rotation/RotationSystem/RotationSystem.h"
#include "Application/Systems/Update/Update/Rotation/LockOnRotationSystem/LockOnRotationSystem.h"
#include "Application/Systems/Update/Update/Acceleration/GravitySystem/GravitySystem.h"
#include "Application/Systems/Update/Update/Move/CharacterMovementSystem/CharacterMovementSystem.h"
#include "Application/Systems/Update/Physics/RayCollisionSystem/RayCollisionSystem.h"
#include "Application/Systems/Update/Physics/Integral/PositionIntegrationSystem/PositionIntegrationSystem.h"
#include "Application/Systems/Update/Physics/Integral/MovementIntegrationSystem/MovementIntegrationSystem.h"
#include "Application/Systems/Update/Camera/TPSSystem/TPSSystem.h"
#include "Application/Systems/Update/Camera/CameraProjUpdateSystem/CameraProjUpdateSystem.h"
#include "Application/Systems/Update/Camera/AimTargetSystem/AimTargetSystem.h"
#include "Application/Systems/Update/PostUpdate/CommitWorldMatrixSystem/CalcMatrixSystem.h"
#include "Application/Systems/Update/PostUpdate/LockOnTargetSystem/LockOnTargetSystem.h"
#include "Application/Systems/Update/PostUpdate/MissileSalvoSystem/MissileSalvoSystem.h"
#include "Application/Systems/Update/PostUpdate/BossMissileSalvoSystem/BossMissileSalvoSystem.h"
#include "Application/Systems/Update/PostUpdate/AnimationSystem/AnimationSystem.h"
#include "Application/Systems/Update/PostUpdate/SkinningSystem/SkinningSystem.h"
#include "Application/Systems/Update/PostUpdate/CalcNodeSystem/CalcNodeSystem.h"
#include "Application/Systems/Update/PostUpdate/FollowAnimationNodeSystem/FollowAnimationNodeSystem.h"
#include "Application/Systems/Draw/PreDraw/CamSetShaderSystem/CamSetShaderSystem.h"
#include "Application/Systems/Draw/Draw/StaticObjectDrawSystem/StaticObjectDrawSystem.h"
#include "Application/Systems/Draw/Draw/DynamicObjectDrawSystem/DynamicObjectDrawSystem.h"
#include "Application/Systems/Draw/Draw/AnimationOptionalDraw/AnimationOptionalDraw.h"
#include "Application/Systems/Draw/Draw/ScreenUIDraw/ScreenUIDrawSystem.h"
#include "Application/Systems/Draw/Draw/RegisterRayWorldSystem/RegisterRayWorldSystem.h"
#include "Application/Systems/Release/AnimationMatrixFreeSystem/AnimationMatrixFreeSystem.h"
#include "Application/Systems/Draw/PostDraw/RegisterPrevWorldMatSystem/RegisterPrevWorldMatSystem.h"
#include "Application/Systems/Init/PostDeserialize/StateMachineFixupSystem/StateMachineFixupSystem.h"
#include "Application/Systems/Update/Update/StateMachineCommitSystem/StateMachineCommitSystem.h"
#include "Application/Systems/Update/PreUpdate/PlayerIntentSystem/PlayerIntentSystem.h"
#include "Application/Components/Resource/ActionStateComponent.h"
#include "Application/Systems/Init/PostDeserialize/ActionStateFixupSystem/ActionStateFixupSystem.h"
#include "Application/Systems/Update/PreUpdate/ActionIntentSystem/ActionIntentSystem.h"
#include "Application/Systems/Update/Update/ActionStateCommitSystem/ActionStateCommitSystem.h"
#include "Application/Systems/Update/Update/ActionBehaviorSystem/ActionBehaviorSystem.h"
#include "Application/Systems/Update/Animation/AnimationStateSystem/AnimationStateSystem.h"
#include "Application/Systems/Update/Update/Move/RobotBoostSystem/RobotBoostSystem.h"
#include "Application/Systems/Update/Update/Move/ChargeDashSystem/ChargeDashSystem.h"
#include "Application/Systems/Draw/Draw/EmitParticlesSystem/EmitParticlesSystem.h"
#include "Application/Systems/Update/Update/Particle/ParticleEmitSystem/ParticleEmitSystem.h"
#include "Application/Systems/Init/PostDeserialize/ParticleFixupSystem/ParticleFixupSystem.h"
#include "Application/Systems/Init/PostDeserialize/EffectFixupSystem/EffectFixupSystem.h"
#include "Application/Systems/Update/Update/Effect/EffectUpdateSystem/EffectUpdateSystem.h"
#include "Application/Systems/Update/Update/Effect/BoosterEffectSystem/BoosterEffectSystem.h"
#include "Application/Systems/Draw/Draw/EffectDrawSystem/EffectDrawSystem.h"
#include "Application/Systems/Update/PreUpdate/UpdateHierarchyDepthSystem/UpdateHierarchyDepthSystem.h"
#include "Application/Systems/Update/PostUpdate/CommitHierarchyWorldMatrixSystem/CommitHierarchyWorldMatrixSystem.h"
#include "../../Systems/Draw/Draw/SkinningRegisterSystem/SkinningRegisterSystem.h"
#include "../../Systems/Draw/Draw/RegisterAnimatedRayWorldSystem/RegisterAnimatedRayWorldSystem.h"
#include "../../Systems/Update/Physics/CapsuleCollisionSystem/CapsuleCollisionSystem.h"
#include "../../Systems/Update/Physics/SphereCollisionSystem/SphereCollisionSystem.h"
#include "../../Systems/Update/Physics/BoxCollisionSystem/BoxCollisionSystem.h"
#include "../../Systems/Update/Physics/OBBCollisionSystem/OBBCollisionSystem.h"
#include "../../Systems/Update/Input/InputActionSystem/InputActionSystem.h"
#include "../../Systems/Update/Update/GunShootSystem/GunShootSystem.h"
#include "../../Systems/Update/PreUpdate/CollisionEventClearSystem/CollisionEventClearSystem.h"
#include "../../Systems/Update/Physics/HitDetectSystem/HitDetectSystem.h"
#include "../../Systems/Update/PostUpdate/ExplodeOnHitSystem/ExplodeOnHitSystem.h"
#include "../../Systems/Update/PostUpdate/LifeTimeSystem/LifeTimeSystem.h"
#include "../../Systems/Update/PreUpdate/AttachmentDispatchSystem/AttachmentDispatchSystem.h"
#include "../../Systems/Update/PreUpdate/SelfWeaponTriggerSystem/SelfWeaponTriggerSystem.h"
#include "../../Systems/Update/PreUpdate/ThrusterEffectSystem/ThrusterEffectSystem.h"
#include "../../Systems/Init/PostDeserialize/AttachmentSlotLinkSystem/AttachmentSlotLinkSystem.h"
#include "../../Systems/Update/Update/SubmitDynamicColliderSystem/SubmitDynamicColliderSystem.h"
#include "../../Systems/Init/Start/AdditivePoseLinkSystem/AdditivePoseLinkSystem.h"
#include "../../Systems/Update/PostUpdate/AdditivePoseSystem/AdditivePoseSystem.h"
#include "../../Systems/Release/AdditivePoseFreeSystem/AdditivePoseFreeSystem.h"
#include "../../Systems/Update/PreUpdate/SearchPlayerSystem/SearchPlayerSystem.h"
#include "../../Systems/Update/PreUpdate/SightStateBridgeSystem/SightStateBridgeSystem.h"
#include "../../Systems/Update/Update/FaceTargetSystem/FaceTargetSystem.h"
#include "../../Systems/Update/PreUpdate/EnemyMoveIntentSystem/EnemyMoveIntentSystem.h"
#include "../../Systems/Update/PreUpdate/LostTargetBridgeSystem/LostTargetBridgeSystem.h"
#include "../../Systems/Update/Update/LookAroundSystem/LookAroundSystem.h"
#include "../../Systems/Update/Update/Move/EnemyMovementSystem/EnemyMovementSystem.h"
#include "../../Systems/Init/PostDeserialize/SoundFixupSystem/SoundFixupSystem.h"
#include "../../Systems/Update/PreUpdate/BoostSoundSystem/BoostSoundSystem.h"
#include "../../Systems/Init/Start/SpawnSoundSystem/SpawnSoundSystem.h"
#include "../../Systems/Release/SoundFreeSystem/SoundFreeSystem.h"
#include "../../Systems/Init/Start/GunStateStartSystem/GunStateStartSystem.h"
#include "../../Systems/Update/PreUpdate/HitEventClearSystem/HitEventClearSystem.h"
#include "../../Systems/Update/PreUpdate/DeathEventClearSystem/DeathEventClearSystem.h"
#include "../../Systems/Update/PreUpdate/EnemyShootIntentSystem/EnemyShootIntentSystem.h"
#include "../../Systems/Update/PreUpdate/BossCombatIntentSystem/BossCombatIntentSystem.h"
#include "../../Systems/Update/PreUpdate/HomingSystem/HomingSystem.h"
#include "../../Systems/Update/PostUpdate/HitSoundSystem/HitSoundSystem.h"
#include "../../Components/Resource/HitSoundComponent.h"
#include "../../Components/Resource/AudioBehaviorComponent.h"
#include "../../Systems/Update/PostUpdate/AudioListenerSystem/AudioListenerSystem.h"
#include "../../Systems/Update/PostUpdate/FlyingSoundSystem/FlyingSoundSystem.h"
#include "../../Components/Resource/AudioListenerComponent.h"
#include "../../Components/Character/FlyingSound.h"
#include "../../InstanceResource/FlyingSoundResource.h"
#include "../../Components/Character/HealthComponent.h"
#include "../../Systems/Init/PostDeserialize/HealthFixupSystem/HealthFixupSystem.h"
#include "../../Systems/Update/PostUpdate/HealthSystem/HealthSystem.h"
#include "../../Systems/Update/PostUpdate/DeathStateSystem/DeathStateSystem.h"
#include "../../Components/Effect/EffectComponent.h"
#include "../../Components/Effect/EffectAssetComponent.h"
#include "../../Components/Common/LifeTimeComponent.h"
#include "../../Components/Character/DeathEffectComponent.h"
#include "../../Components/Effect/ExplosionComponent.h"
#include "../../InstanceResource/DeathEventResource.h"
#include "../../Systems/Update/PostUpdate/DeathEffectSystem/DeathEffectSystem.h"
#include "../../Systems/Update/PostUpdate/ScoreSystem/ScoreSystem.h"
#include "../../Systems/Update/PostUpdate/ExplosionSystem/ExplosionSystem.h"

// リソース関係
#include "Application/InstanceResource/HierarchyResource.h"
#include "Application/InstanceResource/ResourceWaitResource.h"
#include "../../InstanceResource/AdditiveBoneEntry.h"
#include "Application/InstanceResource/HitEventResource.h"

// インプット
#include "Engine/Input/InputCollector/InputCollector.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindowsMouse/InputAxisForWindowsMouse.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForXInput/InputAxisForXInput.h"

#include "Engine/Input/InputDevice/Button/InputButtonForWindows/InputButtonForWindows.h"
#include "Engine/Input/InputDevice/Button/InputButtonForXInput/InputButtonForXInput.h"

// ゲームフロウ
#include "../GameFlowStateMachine/GameFlowStateMachine.h"

#include "../../../Engine/Audio/AudioManager.h"

namespace App::Game
{
	void App::Game::GameManager::Init()
	{

		// ゲームフロウの読み込み
		m_upGameFlowMachine = std::make_unique<GameFlowStateMachine>();
		m_upGameFlowMachine->Load("Asset/Scenes/Flow/Flow.scene");

		// テスト : 音源読み込み
		m_testHandle = Engine::Audio::AudioManager::Instance().RequestSoundInstance("Asset/Sound/TEST/test.wav");

		// ------------------------------------------------------------------
		// ECS外オブジェクト(GameObject)のクラスをメタマネージャーへ登録する。
		// ここで登録した順にタイプインデックスが振られ、シーンの保存/読み込み・
		// エディターの AddObject 一覧で利用される。
		// 新しいオブジェクトクラスを追加したら、ここに RegisterType を足すこと。
		// ------------------------------------------------------------------
		{
			auto& _objRegistry = Engine::GameObject::ObjectMetaRegistry::Instance();
			_objRegistry.RegisterType<App::Object::CombatReticleHUD>("CombatReticleHUD");
			_objRegistry.RegisterType<App::Object::TargetBoxHUD>("TargetBoxHUD");
			_objRegistry.RegisterType<App::Object::SceneSequence>("SceneSequence");
			_objRegistry.RegisterType<App::Object::AimReticleHUD>("AimReticleHUD");
			_objRegistry.RegisterType<App::Object::HitEffectHUD>("HitEffectHUD");
			_objRegistry.RegisterType<App::Object::MissileLockBoxHUD>("MissileLockBoxHUD");
			// 押せるUI。押されて何をするかは SetOnClick で外から差し込む
			_objRegistry.RegisterType<App::Object::UIButton>("UIButton");
			// 置くだけの画像(タイトルの背景など)
			_objRegistry.RegisterType<App::Object::UIImage>("UIImage");
			// タイトル画面の進行役。ボタンへ「押されたらシーンを切り替える」を差し込む
			_objRegistry.RegisterType<App::Object::TitleSequence>("TitleSequence");
			// シーンの環境設定(環境光・平行光・フォグ・空)。シーンに1つ置く。
			// ※ タイプIDは登録順で振られるので、必ず末尾に足すこと
			//    (間に挟むと既存シーンの TypeIndex が全部ずれる)
			_objRegistry.RegisterType<App::Object::SceneAmbientObject>("SceneAmbientObject");
			// スコアの表示。数える側(ScoreSystem)とは分かれていて、ここは出すだけ
			_objRegistry.RegisterType<App::Object::ScoreHUD>("ScoreHUD");
			// リザルト画面の進行役。ホームのボタンへ「押されたらタイトルへ」を差し込む
			_objRegistry.RegisterType<App::Object::ResultSequence>("ResultSequence");
			// ホーム画面の進行役。ステージセレクト(一覧・詳細・出撃)と倉庫のボタンを束ねる
			_objRegistry.RegisterType<App::Object::HomeSequence>("HomeSequence");
			// ポーズ画面の進行役。重ねたシーンを閉じる側(重ねるのは SceneSequence)
			_objRegistry.RegisterType<App::Object::PauseSequence>("PauseSequence");
			// ミッションセレクト。ホームから出し入れされ、選ぶと確認ボックスを出して出撃する
			_objRegistry.RegisterType<App::Object::MissionSelect>("MissionSelect");
		}

		// ワールドの初期化関数登録
		Engine::Scene::SceneManager::Instance().SetWorldInitCallback(
			[](Engine::ECS::World* a_pWorld)
			{
				// ECSにコンポーネントを登録
				a_pWorld->RegisterComponent<PostDeserializeTag>("PostDeserializeTag");
				a_pWorld->RegisterComponent<AwakeTag>("AwakeTag");
				a_pWorld->RegisterComponent<StartTag>("StartTag");
				a_pWorld->RegisterComponent<ActiveTag>("ActiveTag");
				a_pWorld->RegisterComponent<ReleaseTag>("ReleaseTag");
				a_pWorld->RegisterComponent<EnemyTag>("EnemyTag");

				a_pWorld->RegisterComponent<RayTag>("RayTag");

				a_pWorld->RegisterComponent<ActiveCameraTag>("ActiveCameraTag");
				a_pWorld->RegisterComponent<CameraTag>("CameraTag");
				a_pWorld->RegisterComponent<CameraControllTag>("CameraControllTag");
				a_pWorld->RegisterComponent<PlayerControllTag>("PlayerControllTag");

				a_pWorld->RegisterComponent<CameraParamComponent>("CameraParamComponent");
				a_pWorld->RegisterComponent<ProjMatComponent>("ProjMatComponent");
				a_pWorld->RegisterComponent<FocusParamComponent>("FocusParamComponent");
				a_pWorld->RegisterComponent<FollowTargetComponent>("FollowTargetComponent");
				a_pWorld->RegisterComponent<TPSOffsetComponent>("TPSOffsetComponent");
				a_pWorld->RegisterComponent<TPSLookAngleComponent>("TPSLookAngleComponent");
				a_pWorld->RegisterComponent<VelocityComponent>("VelocityComponent");
				a_pWorld->RegisterComponent<GravityComponent>("GravityComponent");
				a_pWorld->RegisterComponent<MovementComponent>("MovementComponent");
				a_pWorld->RegisterComponent<LookAngleComponent>("LookAngleComponent");
				a_pWorld->RegisterComponent<ColliderComponent>("ColliderComponent");
				a_pWorld->RegisterComponent<RayColliderComponent>("RayColliderComponent");
				a_pWorld->RegisterComponent<LocalTransformComponent>("LocalTransformComponent");
				a_pWorld->RegisterComponent<WorldMatrixComponent>("WorldMatrixComponent");
				a_pWorld->RegisterComponent<ModelComponent>("ModelComponent");
				a_pWorld->RegisterComponent<AnimatorComponent>("AnimatorComponent");
				a_pWorld->RegisterComponent<SkeletonPoseComponent>("SkeletonPoseComponent");
				a_pWorld->RegisterComponent<NodePoseComponent>("NodePoseComponent");
				a_pWorld->RegisterComponent<UIComponent>("UIComponent");
				a_pWorld->RegisterComponent<NameComponent>("NameComponent");
				a_pWorld->RegisterComponent<GUIDComponent>("GUIDComponent");
				a_pWorld->RegisterComponent<HierarchyComponent>("HierarchyComponent");
				// 出現させた側(SceneSequence)の印。ウェーブの全滅判定に使う
				a_pWorld->RegisterComponent<SpawnerComponent>("SpawnerComponent");
				a_pWorld->RegisterComponent<FollowAnimationNodeComponent>("FollowAnimationNodeComponent");
				a_pWorld->RegisterComponent<StateMachineComponent>("StateMachineComponent");
				a_pWorld->RegisterComponent<ActionStateComponent>("ActionStateComponent");
				a_pWorld->RegisterComponent<MoveIntentComponent>("MoveIntentComponent");
				a_pWorld->RegisterComponent<PreviousWorldMatrixComponent>("PreviousWorldMatrixComponent");
				a_pWorld->RegisterComponent<BoostComponent>("BoostComponent");
				a_pWorld->RegisterComponent<AttachmentSlotsComponent>("AttachmentSlotsComponent");
				a_pWorld->RegisterComponent<ParticlesComponent>("ParticlesComponent");
				a_pWorld->RegisterComponent<TPSCameraStateComponent>("TPSCameraStateComponent");
				a_pWorld->RegisterComponent<TPSFollowComponent>("TPSFollowComponent");
				a_pWorld->RegisterComponent<CapsuleColliderComponent>("CapsuleColliderComponent");
				a_pWorld->RegisterComponent<SphereColliderComponent>("SphereColliderComponent");
				a_pWorld->RegisterComponent<BoxColliderComponent>("BoxColliderComponent");
				a_pWorld->RegisterComponent<OBBColliderComponent>("OBBColliderComponent");
				a_pWorld->RegisterComponent<ActionIntentComponent>("ActionIntentComponent");
				a_pWorld->RegisterComponent<GunStateComponent>("GunStateComponent");
				// 武器が外から受け取る引き金。持ち主の命令と武器の挙動を分ける受け口
				a_pWorld->RegisterComponent<WeaponTriggerComponent>("WeaponTriggerComponent");
				a_pWorld->RegisterComponent<Engine::ECS::CollisionEvent>("CollisionEvent");
				a_pWorld->RegisterComponent<ExplodeOnHitComponent>("ExplodeOnHitComponent");
				a_pWorld->RegisterComponent<CameraFocusTargetComponent>("CameraFocusTargetComponent");
				// TPSカメラの追従範囲。枠から出たぶんだけカメラを平行移動させる
				a_pWorld->RegisterComponent<CameraDeadZoneComponent>("CameraDeadZoneComponent");
				a_pWorld->RegisterComponent<AdditivePoseComponent>("AdditivePoseComponent");
				a_pWorld->RegisterComponent<AimTargetPosComponent>("AimTargetPosComponent");
				a_pWorld->RegisterComponent<PatrolComponent>("PatrolComponent");
				a_pWorld->RegisterComponent<TargetEntityComponent>("TargetEntityComponent");
				// プレイヤーのレティクル内の敵とロック対象。HUDと旋回が読む
				a_pWorld->RegisterComponent<LockOnTargetComponent>("LockOnTargetComponent");
				// ミサイルの溜め撃ち。コンバットレティクル内の敵を溜めて一斉射する
				a_pWorld->RegisterComponent<MissileLockComponent>("MissileLockComponent");
				// 人型ボスの戦闘設定と機動状態。シーケンスからの戦闘開始命令もここに立つ
				a_pWorld->RegisterComponent<BossComponent>("BossComponent");
				a_pWorld->RegisterComponent<SoundComponent>("SoundComponent");
				a_pWorld->RegisterComponent<HitSoundComponent>("HitSoundComponent");
				// 始動/継続/終了の音をまとめた AudioBehavior アセットを鳴らす
				a_pWorld->RegisterComponent<AudioBehaviorComponent>("AudioBehaviorComponent");
				a_pWorld->RegisterComponent<AudioListenerComponent>("AudioListenerComponent");
				a_pWorld->RegisterComponent<FlyingSoundComponent>("FlyingSoundComponent");
				a_pWorld->RegisterComponent<HealthComponent>("HealthComponent");
				a_pWorld->RegisterComponent<EffectComponent>("EffectComponent");
				// パーティクル+メッシュをまとめた EffectAsset を再生する
				a_pWorld->RegisterComponent<EffectAssetComponent>("EffectAssetComponent");
				a_pWorld->RegisterComponent<LifeTimeComponent>("LifeTimeComponent");
				a_pWorld->RegisterComponent<DeathEffectComponent>("DeathEffectComponent");
				a_pWorld->RegisterComponent<ExplosionComponent>("ExplosionComponent");
				a_pWorld->RegisterComponent<HomingComponent>("HomingComponent");
				a_pWorld->RegisterComponent<ProjectileComponent>("ProjectileComponent");
				// ※ 追加はここから下(末尾)へ。途中に挿すとコンポーネントのタイプIDがずれて
				//    保存済みのプレハブ・シーンが全部壊れる
				a_pWorld->RegisterComponent<BoosterEffectComponent>("BoosterEffectComponent");
				// ジャンプ長押しで溜めて直進するチャージダッシュ
				a_pWorld->RegisterComponent<ChargeDashComponent>("ChargeDashComponent");
				// 倒す相手であることの印と、倒したときに入るスコア
				a_pWorld->RegisterComponent<ScoreTargetComponent>("ScoreTargetComponent");

				// システム登録
				a_pWorld->RegisterSystem<ModelFixupSystem>();
				a_pWorld->RegisterSystem<GUIDFixupSystem>();
				a_pWorld->RegisterSystem<StateMachineFixupSystem>();
				a_pWorld->RegisterSystem<ActionStateFixupSystem>();
				a_pWorld->RegisterSystem<ParticleFixupSystem>();
				a_pWorld->RegisterSystem<EffectFixupSystem>();
				a_pWorld->RegisterSystem<SoundFixupSystem>();
				// 現在体力を最大体力で満たす
				a_pWorld->RegisterSystem<HealthFixupSystem>();
				// リソースの到着待ちゲート。
				// AwakeTag -> StartTag の遷移より前に走らせる必要があるため、
				// Awake フェーズの先頭付近に置くこと
				a_pWorld->RegisterSystem<ModelReadyGateSystem>();
				a_pWorld->RegisterSystem<FollowTargetLinkSystem>();
				a_pWorld->RegisterSystem<AttachmentSlotLinkSystem>();
				a_pWorld->RegisterSystem<HierarchyLinkSystem>();
				// 親モデルの到着待ちゲート。
				// 親IDの解決(HierarchyLinkSystem)より後に走る必要があるため、
				// 必ずこの位置より下に置くこと
				a_pWorld->RegisterSystem<AttachmentReadyGateSystem>();
				a_pWorld->RegisterSystem<PlayerIntentSystem>();
				a_pWorld->RegisterSystem<AttachmentDispatchSystem>();
				// 本体が武器を兼ねているキャラ(銃を子に持たない敵など)の引き金を渡す。
				// 武器が子の場合は上の AttachmentDispatchSystem が受け持つ
				a_pWorld->RegisterSystem<SelfWeaponTriggerSystem>();
				a_pWorld->RegisterSystem<ThrusterEffectSystem>();
				a_pWorld->RegisterSystem<BoostSoundSystem>();
				a_pWorld->RegisterSystem<ActionIntentSystem>();
				a_pWorld->RegisterSystem<SearchPlayerSystem>();
				a_pWorld->RegisterSystem<SightStateBridgeSystem>();
				// 索敵結果(isFind)を敵の発射入力へ。銃が子なら AttachmentDispatchSystem が配信する
				a_pWorld->RegisterSystem<EnemyShootIntentSystem>();
				// ボスの行動決定。プレイヤーの入力と同じ形(視点角/移動/ブースト/発射/狙点)を作る
				a_pWorld->RegisterSystem<BossCombatIntentSystem>();
				// 誘導弾の進行方向決め。速度を書くだけなので Physics の積分より前に置く
				a_pWorld->RegisterSystem<HomingSystem>();
				a_pWorld->RegisterSystem<EnemyMoveIntentSystem>();
				// 見失い探索のフェーズ(EnemyMoveIntentSystem が進める)を FSM パラメータへ
				a_pWorld->RegisterSystem<LostTargetBridgeSystem>();
				a_pWorld->RegisterSystem<StateMachineCommitSystem>();
				a_pWorld->RegisterSystem<ActionStateCommitSystem>();
				a_pWorld->RegisterSystem<RegisterCollisionWorldSystem>();
				a_pWorld->RegisterSystem<CameraStartSystem>();
				a_pWorld->RegisterSystem<AnimationModelStartSystem>();
				a_pWorld->RegisterSystem<AttachmentNodeLinkSystem>();
				a_pWorld->RegisterSystem<AdditivePoseLinkSystem>();
				// 湧いた瞬間に鳴らす音(エフェクト用)。インスタンスは SoundFixupSystem が先に用意する
				a_pWorld->RegisterSystem<SpawnSoundSystem>();
				a_pWorld->RegisterSystem<CamSetShaderSystem>();
				a_pWorld->RegisterSystem<InputMoveSystem>();
				a_pWorld->RegisterSystem<GravitySystem>();
				a_pWorld->RegisterSystem<RotationSystem>();
				// プレイヤーの旋回は ActionState を見て切り替えるので専用システムが持つ
				a_pWorld->RegisterSystem<LockOnRotationSystem>();
				a_pWorld->RegisterSystem<FaceTargetSystem>();
				// 見失い探索中の旋回。視認中(FaceTargetSystem)とは条件が排他
				a_pWorld->RegisterSystem<LookAroundSystem>();
				a_pWorld->RegisterSystem<AnimationStateSystem>();
				a_pWorld->RegisterSystem<AnimationSystem>();
				// AnimationSystem がバインドポーズでリセットした後、
				// CalcNodeSystem が local→world を組む前に加算する必要がある
				a_pWorld->RegisterSystem<AdditivePoseSystem>();
				a_pWorld->RegisterSystem<CalcNodeSystem>();
				a_pWorld->RegisterSystem<SkinningSystem>();
				a_pWorld->RegisterSystem<PositionIntegrationSystem>();
				a_pWorld->RegisterSystem<MovementIntegrationSystem>();
				a_pWorld->RegisterSystem<CharacterMovementSystem>();
				a_pWorld->RegisterSystem<EnemyMovementSystem>();
				a_pWorld->RegisterSystem<ActionBehaviorSystem>();
				a_pWorld->RegisterSystem<TPSSystem>();
				// スピードで動く画角(TPSSystem が fovBoost を書く)を射影行列へ反映する。
				// CameraParamComponent を読むので TPSSystem より後に回る
				a_pWorld->RegisterSystem<CameraProjUpdateSystem>();
				// カメラ姿勢が確定した後に狙点レイを撃つ(TPSSystem より後に登録すること)
				a_pWorld->RegisterSystem<AimTargetSystem>();
				a_pWorld->RegisterSystem<CalcMatrixSystem>();
				// レティクル内の敵集めとロック。ワールド行列を読むので
				// それを書く CalcMatrix / CommitHierarchyWorldMatrix より後ろに回る
				a_pWorld->RegisterSystem<LockOnTargetSystem>();
				a_pWorld->RegisterSystem<MissileSalvoSystem>();
				// ボスのミサイル。撃ち出しはプレイヤーと共通(MissileSalvo)で、溜め方だけが違う
				a_pWorld->RegisterSystem<BossMissileSalvoSystem>();
				a_pWorld->RegisterSystem<RobotBoostSystem>();
				// チャージダッシュ。速度を書く仲間(重力・ブースト)より後に登録して、
				// ダッシュ中はこちらの値が最後に残るようにする
				a_pWorld->RegisterSystem<ChargeDashSystem>();
				a_pWorld->RegisterSystem<FollowAnimationNodeSystem>();
				a_pWorld->RegisterSystem<RayCollisionSystem>();
				a_pWorld->RegisterSystem<StaticObjectDrawSystem>();
				a_pWorld->RegisterSystem<DynamicObjectDrawSystem>();
				a_pWorld->RegisterSystem<AnimationOptionalDrawSystem>();
				a_pWorld->RegisterSystem<ScreenUIDrawSystem>();
				a_pWorld->RegisterSystem<RegisterRayWorldSystem>();
				a_pWorld->RegisterSystem<EmitParticleSystem>();
				a_pWorld->RegisterSystem<ParticleEmitSystem>();
				// エフェクト : 時間を進めるのは Update、出すのは Draw
				a_pWorld->RegisterSystem<EffectUpdateSystem>();
				// ブースターの噴射の置き方と、吹かした瞬間の膨らみをエフェクトへ渡す
				a_pWorld->RegisterSystem<BoosterEffectSystem>();
				a_pWorld->RegisterSystem<EffectDrawSystem>();
				a_pWorld->RegisterSystem<AnimationMatrixFreeSystem>();
				a_pWorld->RegisterSystem<AdditivePoseFreeSystem>();
				a_pWorld->RegisterSystem<SoundFreeSystem>();
				a_pWorld->RegisterSystem<RegisterPrevWorldMatSystem>();
				a_pWorld->RegisterSystem<UpdateHierarchyDepthSystem>();
				a_pWorld->RegisterSystem<CommitHierarchyWorldMatrixSystem>();
				a_pWorld->RegisterSystem<SkinningRegisterSystem>();
				a_pWorld->RegisterSystem<RegisterAnimatedRayWorldSystem>();
				a_pWorld->RegisterSystem<CapsuleCollisionSystem>();
				a_pWorld->RegisterSystem<SphereCollisionSystem>();
				a_pWorld->RegisterSystem<BoxCollisionSystem>();
				a_pWorld->RegisterSystem<OBBCollisionSystem>();
				a_pWorld->RegisterSystem<InputActionSystem>();
				a_pWorld->RegisterSystem<GunShootSystem>();
				a_pWorld->RegisterSystem<SubmitDynamicColliderSystem>();
				a_pWorld->RegisterSystem<CollisionEventClearSystem>();
				a_pWorld->RegisterSystem<HitEventClearSystem>();
				// 死亡イベントも読み手が複数(エフェクトとスコア)になったので、
				// ヒットと同じく捨てる係を分けてある
				a_pWorld->RegisterSystem<DeathEventClearSystem>();
				a_pWorld->RegisterSystem<HitDetectSystem>();
				a_pWorld->RegisterSystem<ExplodeOnHitSystem>();
				// 被弾で体力を削り、尽きたら死亡状態にする(体力持ちは ExplodeOnHit の対象外)
				a_pWorld->RegisterSystem<HealthSystem>();
				// 死亡状態のあいだ入力/AIを止め、指定秒たったら解放予約する。
				// 体力を書く HealthSystem より後に登録すること
				// (同じ PostUpdate 帯で HealthComponent を書く同士なので登録順で並ぶ)
				a_pWorld->RegisterSystem<DeathStateSystem>();
				// 寿命持ち(弾・エフェクトなど)の共通処理。尽きたら自分で消える
				a_pWorld->RegisterSystem<LifeTimeSystem>();
				// 死亡したものの DeathEffect プレハブを出す(死亡を積む側より後ろで回る)
				a_pWorld->RegisterSystem<DeathEffectSystem>();
				// 倒した相手ぶんのスコアを足す(死亡を積む側より後ろで回る)
				a_pWorld->RegisterSystem<ScoreSystem>();
				// 時間差で複数のエフェクトを炊き、出し切ったら自分で消える
				a_pWorld->RegisterSystem<ExplosionSystem>();
				// 3Dサウンドの聞き手。鳴らす側より先に登録して、先にリスナーを更新させる
				a_pWorld->RegisterSystem<AudioListenerSystem>();
				// 被弾音。HitEventResource を読むので Physics より後・クリアより前
				a_pWorld->RegisterSystem<HitSoundSystem>();
				// ミサイル等の飛翔音。消えたエンティティのボイス回収もここで行う
				a_pWorld->RegisterSystem<FlyingSoundSystem>();
				a_pWorld->RegisterSystem<GunStateStartSystem>();

				// インスタンスデータの登録
				a_pWorld->AddResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
				a_pWorld->AddResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();

				a_pWorld->AddResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
				a_pWorld->AddResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				a_pWorld->AddResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();

				a_pWorld->AddResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>();
				a_pWorld->AddResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();
				a_pWorld->AddResource<Engine::Pool::ItemPool<Engine::Animation::SkinningMeshData>>();
				

				// シングルトンインスタンスの登録
				a_pWorld->AddResource<HierarchyResource>();
				a_pWorld->AddResource<ResourceWaitResource>();
				a_pWorld->AddResource<HitEventResource>();
				a_pWorld->AddResource<DeathEventResource>();
				a_pWorld->AddResource<FlyingSoundResource>();

				// 初期化
				a_pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>().Init(10000);
				a_pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>().Init(10000);
				a_pWorld->GetResource<Engine::Pool::RangePool<AdditiveBoneEntry>>().Init(10000);

				a_pWorld->GetResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>().Reserve(100);
				a_pWorld->GetResource<Engine::Pool::ItemPool<Engine::Animation::SkinningMeshData>>().Reserve(100);
				a_pWorld->GetResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();

				a_pWorld->GetResource<HierarchyResource>().isDirty = true;

				// 1フレーム分のヒット数はたかが知れているので少なめに確保
				a_pWorld->GetResource<HitEventResource>().Reserve(256);
				a_pWorld->GetResource<DeathEventResource>().Reserve(64);
			}
		);

		// キーボード
		{
			Engine::Input::InputCollector _keyboard;
			Engine::Input::InputButtonForWindows _add('T');
			_keyboard.AddButton("Add", std::make_shared<Engine::Input::InputButtonForWindows>(_add));
			Engine::Input::InputButtonForWindows _save('K');
			_keyboard.AddButton("Save", std::make_shared<Engine::Input::InputButtonForWindows>(_save));

			// 移動
			Engine::Input::InputAxisForWindows _move('W', 'D', 'S', 'A');
			_keyboard.AddAxis("Move", std::make_shared<Engine::Input::InputAxisForWindows>(_move));
			// ジャンプ
			Engine::Input::InputButtonForWindows _jump(VK_SPACE);
			_keyboard.AddButton("Jump", std::make_shared<Engine::Input::InputButtonForWindows>(_jump));
			// 急降下 : ジャンプ(上昇)の逆で、押している間は下向きの入力になる
			// (エディターの複数選択も LCtrl だが、あちらは ImGui 側で見ているので共存する)
			Engine::Input::InputButtonForWindows _dive(VK_LCONTROL);
			_keyboard.AddButton("Dive", std::make_shared<Engine::Input::InputButtonForWindows>(_dive));
			// ブースト
			Engine::Input::InputButtonForWindows _boost(VK_LSHIFT);
			_keyboard.AddButton("Boost", std::make_shared<Engine::Input::InputButtonForWindows>(_boost));
			// 視点
			Engine::Input::InputAxisForWindows _look(VK_UP, VK_RIGHT, VK_DOWN, VK_LEFT);
			_keyboard.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForWindows>(_look));


			Engine::Input::InputButtonForWindows _debugCamUp('E');
			_keyboard.AddButton("FreeCamUp", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamUp));
			Engine::Input::InputButtonForWindows _debugCamDown('Q');
			_keyboard.AddButton("FreeCamDown", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamDown));

			// テスト用ボタン
			Engine::Input::InputButtonForWindows _test('T');
			_keyboard.AddButton("Test", std::make_shared<Engine::Input::InputButtonForWindows>(_test));

			// シーン遷移用
			Engine::Input::InputButtonForWindows _scene('R');
			_keyboard.AddButton("Scene", std::make_shared<Engine::Input::InputButtonForWindows>(_scene));

			// ポーズ : ゲーム中はポーズ画面を重ね、ポーズ中は閉じて戻る。
			// 拾うのは重ねる側(SceneSequence)と閉じる側(PauseSequence)の2つで、
			// どちらも「一番上のシーン」しか更新されないので取り合いにならない
			Engine::Input::InputButtonForWindows _pause(VK_ESCAPE);
			_keyboard.AddButton("Pause", std::make_shared<Engine::Input::InputButtonForWindows>(_pause));

			// ---- マウスボタン ----
			// 武器 : 左クリックで左手、右クリックで右手。
			// 撃てるかどうかは武器側(GunStateComponent / GunShootSystem)の担当で、
			// ここで作るのは「押されている」という命令だけ
			Engine::Input::InputButtonForWindows _shootLeft(VK_LBUTTON);
			_keyboard.AddButton("ShootLeft", std::make_shared<Engine::Input::InputButtonForWindows>(_shootLeft));
			Engine::Input::InputButtonForWindows _shootRight(VK_RBUTTON);
			_keyboard.AddButton("ShootRight", std::make_shared<Engine::Input::InputButtonForWindows>(_shootRight));

			// UIのボタン押下。UIButton が既定で見に行くアクション名
			// (左手の武器と同じ左クリックだが、意味が別なので名前を分けておく)
			Engine::Input::InputButtonForWindows _uiClick(VK_LBUTTON);
			_keyboard.AddButton("UIClick", std::make_shared<Engine::Input::InputButtonForWindows>(_uiClick));

			// ミサイル : 押している間ターゲットを溜め、離すと一斉射
			// (デバッグカメラの FreeCamUp と同じ E キー。使う場面が別なので共存させる)
			Engine::Input::InputButtonForWindows _missile('E');
			_keyboard.AddButton("Missile", std::make_shared<Engine::Input::InputButtonForWindows>(_missile));

			Engine::Input::InputManager::Instance().AddDevice("Keyboard", std::make_unique<Engine::Input::InputCollector>(_keyboard));
		}
		// マウス
		{
			// 視点
			Engine::Input::InputCollector _mouse;
			_mouse.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForWindowsMouse>());

			Engine::Input::InputManager::Instance().AddDevice("Mouse", std::make_unique<Engine::Input::InputCollector>(_mouse));

		}
		// コントローラー
		{
			//Engine::Input::InputCollector _cont;
			//_cont.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForXInput>(0,false));
			//_cont.AddAxis("Move", std::make_shared<Engine::Input::InputAxisForXInput>(0,true));

			//Engine::Input::InputManager::Instance().AddDevice("Controller", std::make_unique<Engine::Input::InputCollector>(_cont));
		}

		// 最初のシーンを挿入
		Engine::GUID _initScene = m_upGameFlowMachine->Start();
		if (_initScene != Engine::DefaultGUID)
		{
			Engine::Scene::SceneManager::Instance().SetNextScene(_initScene, Engine::Scene::SceneChangeType::Push);
		}
		else
		{
			ENGINE_ERRLOG(false,"初めのシーンが見つかりません");
		}

		// エディター関数登録
		Engine::Editor::MainEditor::Instance().RegisterEditFunc(
			[&]()
			{
				if (ImGui::Begin("GameFlowEdit"))
				{
					if(m_upGameFlowMachine)
					{
						m_upGameFlowMachine->EditImGui();
					}
				}
				ImGui::End();
			}
		);
	}
	void GameManager::Update(float a_dt)
	{	
		m_upGameFlowMachine->SetTrigger("ON_START");

		if (Engine::Input::InputManager::Instance().IsPress("Scene"))
		{
			m_upGameFlowMachine->SetTrigger("ToTitle");
		}

		if (Engine::Input::InputManager::Instance().IsPress("Test"))
		{
			auto* _pSoundInstance = Engine::Audio::AudioManager::Instance().RefInstance(m_testHandle);
			_pSoundInstance->Play();
		}
			
		// 遷移チェック
		Engine::GUID _nextScene;
		if (m_upGameFlowMachine->Evaluate(_nextScene))
		{
			// 遷移が発生したので、指定された新しいシーンをロード！
			Engine::Scene::SceneManager::Instance().SetNextScene(_nextScene, Engine::Scene::SceneChangeType::Replace);
		}

		// タイマー開始
		Engine::Editor::MainEditor::Instance().StartTimer("GameUpdate");
		
		// シーンマネージャーの更新
		Engine::Scene::SceneManager::Instance().Update(a_dt);

		// タイマーストップ
		Engine::Editor::MainEditor::Instance().StopTimer("GameUpdate");
	}
	void GameManager::Draw()
	{
		// タイマー開始
		Engine::Editor::MainEditor::Instance().StartTimer("GameDraw");

		// シーンの描画 : 描画命令を積むだけで実行はしない
		Engine::Scene::SceneManager::Instance().Draw();

		// タイマーストップ
		Engine::Editor::MainEditor::Instance().StopTimer("GameDraw");
	}
	void GameManager::Release()
	{}
	void GameManager::FireGlobalEvent(const std::string & a_eventName)
	{}
	void GameManager::EditDraw()
	{

	}
	GameManager::GameManager()
	{}
	GameManager::~GameManager()
	{}
}