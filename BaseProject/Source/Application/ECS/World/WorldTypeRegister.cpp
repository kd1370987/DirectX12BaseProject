#include "WorldTypeRegister.h"

#include "World.h"

// コンポーネント関係
// システムフェーズタグ
#include "Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "Application/Components/Tag/SystemPhaseTag/AwakeTag.h"
#include "Application/Components/Tag/SystemPhaseTag/StartTag.h"
#include "Application/Components/Tag/SystemPhaseTag/ActiveTag.h"

// コンポーネント
#include "Application/Components/Tag/RenderTag/RayTag.h"
#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Tag/CameraControllTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/RadialBlurComponent.h"
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
#include "Application/Components/Light/PointLightComponent.h"
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
#include "../../Components/Character/CloseCombatComponent.h"
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
#include "Application/Systems/Update/PreUpdate/MainCameraSystem/MainCameraSystem.h"
#include "Application/Systems/Update/Camera/CameraProjUpdateSystem/CameraProjUpdateSystem.h"
#include "Application/Systems/Update/Camera/RadialBlurSpeedSystem/RadialBlurSpeedSystem.h"
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
#include "Application/Systems/Draw/PreDraw/PointLightSystem/PointLightSystem.h"
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
#include "../../Systems/Update/PreUpdate/CloseCombatIntentSystem/CloseCombatIntentSystem.h"
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
#include "../../InstanceResource/WaveAnnounceResource.h"
#include "../../Systems/Update/PostUpdate/DeathEffectSystem/DeathEffectSystem.h"
#include "../../Systems/Update/PostUpdate/ScoreSystem/ScoreSystem.h"
#include "../../Systems/Update/PostUpdate/ExplosionSystem/ExplosionSystem.h"

// リソース関係
#include "Application/InstanceResource/HierarchyResource.h"
#include "Application/InstanceResource/SingletonEntityResource.h"
#include "Application/InstanceResource/ResourceWaitResource.h"
#include "../../InstanceResource/AdditiveBoneEntry.h"
#include "Application/InstanceResource/HitEventResource.h"

namespace App::ECS
{
	void RegisterGameTypes(World& a_world)
	{
		// ECSにコンポーネントを登録
		a_world.RegisterComponent<PostDeserializeTag>("PostDeserializeTag");
		a_world.RegisterComponent<AwakeTag>("AwakeTag");
		a_world.RegisterComponent<StartTag>("StartTag");
		a_world.RegisterComponent<ActiveTag>("ActiveTag");
		a_world.RegisterComponent<ReleaseTag>("ReleaseTag");
		a_world.RegisterComponent<EnemyTag>("EnemyTag");

		a_world.RegisterComponent<RayTag>("RayTag");

		a_world.RegisterComponent<CameraTag>("CameraTag");
		a_world.RegisterComponent<CameraControllTag>("CameraControllTag");
		a_world.RegisterComponent<PlayerControllTag>("PlayerControllTag");

		a_world.RegisterComponent<CameraParamComponent>("CameraParamComponent");
		a_world.RegisterComponent<ProjMatComponent>("ProjMatComponent");
		a_world.RegisterComponent<FocusParamComponent>("FocusParamComponent");
		a_world.RegisterComponent<RadialBlurComponent>("RadialBlurComponent");
		a_world.RegisterComponent<FollowTargetComponent>("FollowTargetComponent");
		a_world.RegisterComponent<TPSOffsetComponent>("TPSOffsetComponent");
		a_world.RegisterComponent<TPSLookAngleComponent>("TPSLookAngleComponent");
		a_world.RegisterComponent<VelocityComponent>("VelocityComponent");
		a_world.RegisterComponent<GravityComponent>("GravityComponent");
		a_world.RegisterComponent<MovementComponent>("MovementComponent");
		a_world.RegisterComponent<LookAngleComponent>("LookAngleComponent");
		a_world.RegisterComponent<ColliderComponent>("ColliderComponent");
		a_world.RegisterComponent<RayColliderComponent>("RayColliderComponent");
		a_world.RegisterComponent<LocalTransformComponent>("LocalTransformComponent");
		a_world.RegisterComponent<WorldMatrixComponent>("WorldMatrixComponent");
		a_world.RegisterComponent<ModelComponent>("ModelComponent");
		a_world.RegisterComponent<AnimatorComponent>("AnimatorComponent");
		a_world.RegisterComponent<SkeletonPoseComponent>("SkeletonPoseComponent");
		a_world.RegisterComponent<NodePoseComponent>("NodePoseComponent");
		a_world.RegisterComponent<UIComponent>("UIComponent");
		a_world.RegisterComponent<NameComponent>("NameComponent");
		a_world.RegisterComponent<GUIDComponent>("GUIDComponent");
		a_world.RegisterComponent<HierarchyComponent>("HierarchyComponent");
		// 出現させた側(SceneSequence)の印。ウェーブの全滅判定に使う
		a_world.RegisterComponent<SpawnerComponent>("SpawnerComponent");
		a_world.RegisterComponent<FollowAnimationNodeComponent>("FollowAnimationNodeComponent");
		a_world.RegisterComponent<StateMachineComponent>("StateMachineComponent");
		a_world.RegisterComponent<ActionStateComponent>("ActionStateComponent");
		a_world.RegisterComponent<MoveIntentComponent>("MoveIntentComponent");
		a_world.RegisterComponent<PreviousWorldMatrixComponent>("PreviousWorldMatrixComponent");
		a_world.RegisterComponent<BoostComponent>("BoostComponent");
		a_world.RegisterComponent<AttachmentSlotsComponent>("AttachmentSlotsComponent");
		a_world.RegisterComponent<ParticlesComponent>("ParticlesComponent");
		a_world.RegisterComponent<TPSCameraStateComponent>("TPSCameraStateComponent");
		a_world.RegisterComponent<TPSFollowComponent>("TPSFollowComponent");
		a_world.RegisterComponent<CapsuleColliderComponent>("CapsuleColliderComponent");
		a_world.RegisterComponent<SphereColliderComponent>("SphereColliderComponent");
		a_world.RegisterComponent<BoxColliderComponent>("BoxColliderComponent");
		a_world.RegisterComponent<OBBColliderComponent>("OBBColliderComponent");
		a_world.RegisterComponent<ActionIntentComponent>("ActionIntentComponent");
		a_world.RegisterComponent<GunStateComponent>("GunStateComponent");
		// 武器が外から受け取る引き金。持ち主の命令と武器の挙動を分ける受け口
		a_world.RegisterComponent<WeaponTriggerComponent>("WeaponTriggerComponent");
		a_world.RegisterComponent<Engine::ECS::CollisionEvent>("CollisionEvent");
		a_world.RegisterComponent<ExplodeOnHitComponent>("ExplodeOnHitComponent");
		a_world.RegisterComponent<CameraFocusTargetComponent>("CameraFocusTargetComponent");
		// TPSカメラの追従範囲。枠から出たぶんだけカメラを平行移動させる
		a_world.RegisterComponent<CameraDeadZoneComponent>("CameraDeadZoneComponent");
		a_world.RegisterComponent<AdditivePoseComponent>("AdditivePoseComponent");
		a_world.RegisterComponent<AimTargetPosComponent>("AimTargetPosComponent");
		// 近距離型の敵の「足を止めて撃つ / 撃たずに動き直す」のリズム
		a_world.RegisterComponent<CloseCombatComponent>("CloseCombatComponent");
		a_world.RegisterComponent<PatrolComponent>("PatrolComponent");
		a_world.RegisterComponent<TargetEntityComponent>("TargetEntityComponent");
		// プレイヤーのレティクル内の敵とロック対象。HUDと旋回が読む
		a_world.RegisterComponent<LockOnTargetComponent>("LockOnTargetComponent");
		// ミサイルの溜め撃ち。コンバットレティクル内の敵を溜めて一斉射する
		a_world.RegisterComponent<MissileLockComponent>("MissileLockComponent");
		// 人型ボスの戦闘設定と機動状態。シーケンスからの戦闘開始命令もここに立つ
		a_world.RegisterComponent<BossComponent>("BossComponent");
		a_world.RegisterComponent<SoundComponent>("SoundComponent");
		a_world.RegisterComponent<HitSoundComponent>("HitSoundComponent");
		// 始動/継続/終了の音をまとめた AudioBehavior アセットを鳴らす
		a_world.RegisterComponent<AudioBehaviorComponent>("AudioBehaviorComponent");
		a_world.RegisterComponent<AudioListenerComponent>("AudioListenerComponent");
		a_world.RegisterComponent<FlyingSoundComponent>("FlyingSoundComponent");
		a_world.RegisterComponent<HealthComponent>("HealthComponent");
		a_world.RegisterComponent<EffectComponent>("EffectComponent");
		// パーティクル+メッシュをまとめた EffectAsset を再生する
		a_world.RegisterComponent<EffectAssetComponent>("EffectAssetComponent");
		a_world.RegisterComponent<LifeTimeComponent>("LifeTimeComponent");
		a_world.RegisterComponent<DeathEffectComponent>("DeathEffectComponent");
		a_world.RegisterComponent<ExplosionComponent>("ExplosionComponent");
		a_world.RegisterComponent<HomingComponent>("HomingComponent");
		a_world.RegisterComponent<ProjectileComponent>("ProjectileComponent");
		// ※ 追加はここから下(末尾)へ。途中に挿すとコンポーネントのタイプIDがずれて
		//    保存済みのプレハブ・シーンが全部壊れる
		a_world.RegisterComponent<BoosterEffectComponent>("BoosterEffectComponent");
		// ジャンプ長押しで溜めて直進するチャージダッシュ
		a_world.RegisterComponent<ChargeDashComponent>("ChargeDashComponent");
		// 倒す相手であることの印と、倒したときに入るスコア
		a_world.RegisterComponent<ScoreTargetComponent>("ScoreTargetComponent");
		// エンティティの位置を光源にする点光源。実体は LightManager のプールにある
		a_world.RegisterComponent<PointLightComponent>("PointLightComponent");

		// システム登録
		a_world.RegisterSystem<ModelFixupSystem>();
		a_world.RegisterSystem<GUIDFixupSystem>();
		a_world.RegisterSystem<StateMachineFixupSystem>();
		a_world.RegisterSystem<ActionStateFixupSystem>();
		a_world.RegisterSystem<ParticleFixupSystem>();
		a_world.RegisterSystem<EffectFixupSystem>();
		a_world.RegisterSystem<SoundFixupSystem>();
		// 現在体力を最大体力で満たす
		a_world.RegisterSystem<HealthFixupSystem>();
		// リソースの到着待ちゲート。
		// AwakeTag -> StartTag の遷移より前に走らせる必要があるため、
		// Awake フェーズの先頭付近に置くこと
		a_world.RegisterSystem<ModelReadyGateSystem>();
		a_world.RegisterSystem<FollowTargetLinkSystem>();
		a_world.RegisterSystem<AttachmentSlotLinkSystem>();
		a_world.RegisterSystem<HierarchyLinkSystem>();
		// 親モデルの到着待ちゲート。
		// 親IDの解決(HierarchyLinkSystem)より後に走る必要があるため、
		// 必ずこの位置より下に置くこと
		a_world.RegisterSystem<AttachmentReadyGateSystem>();
		a_world.RegisterSystem<PlayerIntentSystem>();
		a_world.RegisterSystem<AttachmentDispatchSystem>();
		// 本体が武器を兼ねているキャラ(銃を子に持たない敵など)の引き金を渡す。
		// 武器が子の場合は上の AttachmentDispatchSystem が受け持つ
		a_world.RegisterSystem<SelfWeaponTriggerSystem>();
		a_world.RegisterSystem<ThrusterEffectSystem>();
		a_world.RegisterSystem<BoostSoundSystem>();
		a_world.RegisterSystem<ActionIntentSystem>();
		a_world.RegisterSystem<SearchPlayerSystem>();
		a_world.RegisterSystem<SightStateBridgeSystem>();
		// 索敵結果(isFind)を敵の発射入力へ。銃が子なら AttachmentDispatchSystem が配信する
		a_world.RegisterSystem<EnemyShootIntentSystem>();
		// ボスの行動決定。プレイヤーの入力と同じ形(視点角/移動/ブースト/発射/狙点)を作る
		a_world.RegisterSystem<BossCombatIntentSystem>();
		// 誘導弾の進行方向決め。速度を書くだけなので Physics の積分より前に置く
		a_world.RegisterSystem<HomingSystem>();
		a_world.RegisterSystem<EnemyMoveIntentSystem>();
		// 近距離型の敵の撃つ/動くのリズム。
		// EnemyMoveIntentSystem が書いた移動入力を攻撃圏の中だけ上書きするので、
		// 必ずあちらの後ろに置くこと(PatrolComponent を読んで辺は張ってある)
		a_world.RegisterSystem<CloseCombatIntentSystem>();
		// 見失い探索のフェーズ(EnemyMoveIntentSystem が進める)を FSM パラメータへ
		a_world.RegisterSystem<LostTargetBridgeSystem>();
		a_world.RegisterSystem<StateMachineCommitSystem>();
		a_world.RegisterSystem<ActionStateCommitSystem>();
		a_world.RegisterSystem<RegisterCollisionWorldSystem>();
		a_world.RegisterSystem<CameraStartSystem>();
		a_world.RegisterSystem<AnimationModelStartSystem>();
		a_world.RegisterSystem<AttachmentNodeLinkSystem>();
		a_world.RegisterSystem<AdditivePoseLinkSystem>();
		// 湧いた瞬間に鳴らす音(エフェクト用)。インスタンスは SoundFixupSystem が先に用意する
		a_world.RegisterSystem<SpawnSoundSystem>();
		a_world.RegisterSystem<CamSetShaderSystem>();
		// 点光源の位置と設定値を LightManager へ送る。
		// GPUバッファへ詰め直されるのは描画フェーズの後なので、この帯で間に合う
		a_world.RegisterSystem<PointLightSystem>();
		a_world.RegisterSystem<InputMoveSystem>();
		a_world.RegisterSystem<GravitySystem>();
		a_world.RegisterSystem<RotationSystem>();
		// プレイヤーの旋回は ActionState を見て切り替えるので専用システムが持つ
		a_world.RegisterSystem<LockOnRotationSystem>();
		a_world.RegisterSystem<FaceTargetSystem>();
		// 見失い探索中の旋回。視認中(FaceTargetSystem)とは条件が排他
		a_world.RegisterSystem<LookAroundSystem>();
		a_world.RegisterSystem<AnimationStateSystem>();
		a_world.RegisterSystem<AnimationSystem>();
		// AnimationSystem がバインドポーズでリセットした後、
		// CalcNodeSystem が local→world を組む前に加算する必要がある
		a_world.RegisterSystem<AdditivePoseSystem>();
		a_world.RegisterSystem<CalcNodeSystem>();
		a_world.RegisterSystem<SkinningSystem>();
		a_world.RegisterSystem<PositionIntegrationSystem>();
		a_world.RegisterSystem<MovementIntegrationSystem>();
		a_world.RegisterSystem<CharacterMovementSystem>();
		a_world.RegisterSystem<EnemyMovementSystem>();
		a_world.RegisterSystem<ActionBehaviorSystem>();
		a_world.RegisterSystem<TPSSystem>();
		// スピードで動く画角(TPSSystem が fovBoost を書く)を射影行列へ反映する。
		// CameraParamComponent を読むので TPSSystem より後に回る
		a_world.RegisterSystem<CameraProjUpdateSystem>();
		// スピードに応じたラジアルブラーの強さ。
		// 画角と同じ speed01 を読むので、それを書く TPSSystem より後に回る
		a_world.RegisterSystem<RadialBlurSpeedSystem>();
		// 映すカメラを1台選んで SingletonEntityResource へ置く。
		// 使う側(狙点・ロックオン・描画のカメラ設定)より手前の帯(PreUpdate)で回る
		a_world.RegisterSystem<MainCameraSystem>();
		// カメラ姿勢が確定した後に狙点レイを撃つ(TPSSystem より後に登録すること)
		a_world.RegisterSystem<AimTargetSystem>();
		a_world.RegisterSystem<CalcMatrixSystem>();
		// レティクル内の敵集めとロック。ワールド行列を読むので
		// それを書く CalcMatrix / CommitHierarchyWorldMatrix より後ろに回る
		a_world.RegisterSystem<LockOnTargetSystem>();
		a_world.RegisterSystem<MissileSalvoSystem>();
		// ボスのミサイル。撃ち出しはプレイヤーと共通(MissileSalvo)で、溜め方だけが違う
		a_world.RegisterSystem<BossMissileSalvoSystem>();
		a_world.RegisterSystem<RobotBoostSystem>();
		// チャージダッシュ。速度を書く仲間(重力・ブースト)より後に登録して、
		// ダッシュ中はこちらの値が最後に残るようにする
		a_world.RegisterSystem<ChargeDashSystem>();
		a_world.RegisterSystem<FollowAnimationNodeSystem>();
		a_world.RegisterSystem<RayCollisionSystem>();
		a_world.RegisterSystem<StaticObjectDrawSystem>();
		a_world.RegisterSystem<DynamicObjectDrawSystem>();
		a_world.RegisterSystem<AnimationOptionalDrawSystem>();
		a_world.RegisterSystem<ScreenUIDrawSystem>();
		a_world.RegisterSystem<RegisterRayWorldSystem>();
		a_world.RegisterSystem<EmitParticleSystem>();
		a_world.RegisterSystem<ParticleEmitSystem>();
		// エフェクト : 時間を進めるのは Update、出すのは Draw
		a_world.RegisterSystem<EffectUpdateSystem>();
		// ブースターの噴射の置き方と、吹かした瞬間の膨らみをエフェクトへ渡す
		a_world.RegisterSystem<BoosterEffectSystem>();
		a_world.RegisterSystem<EffectDrawSystem>();
		a_world.RegisterSystem<AnimationMatrixFreeSystem>();
		a_world.RegisterSystem<AdditivePoseFreeSystem>();
		a_world.RegisterSystem<SoundFreeSystem>();
		a_world.RegisterSystem<RegisterPrevWorldMatSystem>();
		a_world.RegisterSystem<UpdateHierarchyDepthSystem>();
		a_world.RegisterSystem<CommitHierarchyWorldMatrixSystem>();
		a_world.RegisterSystem<SkinningRegisterSystem>();
		a_world.RegisterSystem<RegisterAnimatedRayWorldSystem>();
		a_world.RegisterSystem<CapsuleCollisionSystem>();
		a_world.RegisterSystem<SphereCollisionSystem>();
		a_world.RegisterSystem<BoxCollisionSystem>();
		a_world.RegisterSystem<OBBCollisionSystem>();
		a_world.RegisterSystem<InputActionSystem>();
		a_world.RegisterSystem<GunShootSystem>();
		a_world.RegisterSystem<SubmitDynamicColliderSystem>();
		a_world.RegisterSystem<CollisionEventClearSystem>();
		a_world.RegisterSystem<HitEventClearSystem>();
		// 死亡イベントも読み手が複数(エフェクトとスコア)になったので、
		// ヒットと同じく捨てる係を分けてある
		a_world.RegisterSystem<DeathEventClearSystem>();
		a_world.RegisterSystem<HitDetectSystem>();
		a_world.RegisterSystem<ExplodeOnHitSystem>();
		// 被弾で体力を削り、尽きたら死亡状態にする(体力持ちは ExplodeOnHit の対象外)
		a_world.RegisterSystem<HealthSystem>();
		// 死亡状態のあいだ入力/AIを止め、指定秒たったら解放予約する。
		// 体力を書く HealthSystem より後に登録すること
		// (同じ PostUpdate 帯で HealthComponent を書く同士なので登録順で並ぶ)
		a_world.RegisterSystem<DeathStateSystem>();
		// 寿命持ち(弾・エフェクトなど)の共通処理。尽きたら自分で消える
		a_world.RegisterSystem<LifeTimeSystem>();
		// 死亡したものの DeathEffect プレハブを出す(死亡を積む側より後ろで回る)
		a_world.RegisterSystem<DeathEffectSystem>();
		// 倒した相手ぶんのスコアを足す(死亡を積む側より後ろで回る)
		a_world.RegisterSystem<ScoreSystem>();
		// 時間差で複数のエフェクトを炊き、出し切ったら自分で消える
		a_world.RegisterSystem<ExplosionSystem>();
		// 3Dサウンドの聞き手。鳴らす側より先に登録して、先にリスナーを更新させる
		a_world.RegisterSystem<AudioListenerSystem>();
		// 被弾音。HitEventResource を読むので Physics より後・クリアより前
		a_world.RegisterSystem<HitSoundSystem>();
		// ミサイル等の飛翔音。消えたエンティティのボイス回収もここで行う
		a_world.RegisterSystem<FlyingSoundSystem>();
		a_world.RegisterSystem<GunStateStartSystem>();

		// インスタンスデータの登録
		a_world.AddResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
		a_world.AddResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();

		a_world.AddResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
		a_world.AddResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
		a_world.AddResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();

		a_world.AddResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>();
		a_world.AddResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();
		a_world.AddResource<Engine::Pool::ItemPool<Engine::Animation::SkinningMeshData>>();
	

		// シングルトンインスタンスの登録
		a_world.AddResource<HierarchyResource>();
		a_world.AddResource<SingletonEntityResource>();
		a_world.AddResource<ResourceWaitResource>();
		a_world.AddResource<HitEventResource>();
		a_world.AddResource<DeathEventResource>();
		a_world.AddResource<WaveAnnounceResource>();
		a_world.AddResource<FlyingSoundResource>();

		// 初期化
		a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>().Init(10000);
		a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>().Init(10000);
		a_world.GetResource<Engine::Pool::RangePool<AdditiveBoneEntry>>().Init(10000);

		a_world.GetResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>().Reserve(100);
		a_world.GetResource<Engine::Pool::ItemPool<Engine::Animation::SkinningMeshData>>().Reserve(100);
		a_world.GetResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();

		a_world.GetResource<HierarchyResource>().isDirty = true;

		// 1フレーム分のヒット数はたかが知れているので少なめに確保
		a_world.GetResource<HitEventResource>().Reserve(256);
		a_world.GetResource<DeathEventResource>().Reserve(64);
	}
}
