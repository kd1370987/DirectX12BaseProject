#include "WorldTypeRegister.h"

#include "APPWorld.h"

// コンポーネント関係
// システムフェーズタグ
#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Core/PhaseTag/AwakeTag.h"
#include "Application/Components/Core/PhaseTag/StartTag.h"
#include "Application/Components/Core/PhaseTag/ActiveTag.h"

// コンポーネント
#include "Application/Components/Render/RayTag.h"
#include "Application/Components/Camera/CameraTag.h"
#include "Application/Components/Input/PlayerControllTag.h"
#include "Application/Components/Camera/CameraControllTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/RadialBlurComponent.h"
#include "Application/Components/Camera/FishEyeComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"
#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "Application/Components/Camera/TPSLookAngleComponent.h"
#include "Application/Components/Movement/GravityComponent.h"
#include "Application/Components/Movement/VelocityComponent.h"
#include "Application/Components/Movement/MovementComponent.h"
#include "Application/Components/Movement/LookAngleComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Movement/MoveIntentComponent.h"
#include "Application/Components/Physics/Collider.h"
#include "Application/Components/Physics/RayCollider.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Render/EmissiveOverrideComponent.h"
#include "Application/Components/Physics/GroundStateComponent.h"
#include "Application/Components/Boid/BoidWaveStateComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/SkeletonPoseComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"
#include "Application/Components/Render/UIComponent.h"
#include "Application/Components/Animation/StateMachineComponent.h"
#include "Application/Components/Core/GUIDComponent.h"
#include "Application/Components/Core/NameComponent.h"
#include "Application/Components/Transform/HierarchyComponent.h"
#include "Application/Components/Transform/FollowAnimationNodeComponent.h"
#include "Application/Components/Core/SpawnerComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Movement/BoostComponent.h"
#include "Application/Components/Effect/BoosterEffectComponent.h"
#include "Application/Components/Movement/ChargeDashComponent.h"
#include "Application/Components/Combat/ScoreTargetComponent.h"
#include "Application/Components/Render/PointLightComponent.h"
#include "Application/Components/Attachment/AttachmentSlotsComponent.h"
#include "Application/Components/Effect/ParticlesComponent.h"
#include "Application/Components/Camera/TPSCameraStateComponent.h"
#include "Application/Components/Camera/TPSFollowComponent.h"
#include "Application/Components/Physics/SphereCollider.h"
#include "Application/Components/Enemy/EnemyTag.h"
#include "Application/Components/Physics/CapsuleCollider.h"
#include "Application/Components/Combat/ActionIntentComponent.h"
#include "Application/Components/Weapon/GunStateComponent.h"
#include "Application/Components/Weapon/WeaponTriggerComponent.h"
#include "Engine/ECS/Component/CollisionEvent.h"
#include "Application/Components/Combat/ExplodeOnHitComponent.h"
#include "Application/Components/Camera/CameraFocusTargetComponent.h"
#include "Application/Components/Camera/CameraDeadZoneComponent.h"
#include "Application/Components/Animation/AdditivePoseComponent.h"
#include "Application/Components/Combat/AimTargetPosComponent.h"
#include "Application/Components/Combat/TargetEntityComponent.h"
#include "Application/Components/Combat/LockOnTargetComponent.h"
#include "Application/Components/Audio/SoundComponent.h"
#include "Application/Components/Enemy/PatrolComponent.h"
#include "Application/Components/Enemy/CloseCombatComponent.h"
#include "Application/Components/Weapon/HomingComponent.h"
#include "Application/Components/Weapon/ProjectileComponent.h"
#include "Application/Components/Weapon/MissileLockComponent.h"
#include "Application/Components/Boss/BossComponent.h"
#include "Application/Components/Boid/BoidComponent.h"
#include "Application/Components/Boid/BoidLeaderComponent.h"
#include "Application/Components/Boid/PlatoonLeaderComponent.h"
#include "Application/Components/Boid/BoidSpownerComponent.h"
#include "Application/Components/Boid/SwarmBossBoidTag.h"
#include "Application/Components/Boid/SerchGroundComponent.h"
#include "Application/Components/Boid/WarmGroundEffectComponent.h"
#include "Application/Components/Effect/DebrisEmitterComponent.h"
#include "Application/Components/Effect/BallisticComponent.h"
#include "Application/Components/Boid/BoidContactDamageComponent.h"

// システム関連
#include "Application/Systems/Render/ModelFixupSystem.h"
#include "Application/Systems/Core/GUIDFixupSystem.h"
#include "Application/Systems/Render/ModelReadyGateSystem.h"
#include "Application/Systems/Attachment/AttachmentReadyGateSystem.h"
#include "Application/Systems/Camera/FollowTargetLinkSystem.h"
#include "Application/Systems/Transform/HierarchyLinkSystem.h"
#include "Application/Systems/Camera/CameraStartSystem.h"
#include "Application/Systems/Animation/AnimationModelStartSystem.h"
#include "Application/Systems/Physics/RegisterPhysicsBodySystem.h"
#include "Application/Systems/Attachment/AttachmentNodeLinkSystem.h"
#include "Application/Systems/Input/InputMoveSystem.h"
#include "Application/Systems/Movement/RotationSystem.h"
#include "Application/Systems/Movement/LockOnRotationSystem.h"
#include "Application/Systems/Movement/GravitySystem.h"
#include "Application/Systems/Movement/CharacterMovementSystem.h"
#include "Application/Systems/Physics/RayCollisionSystem.h"
#include "Application/Systems/Physics/PositionIntegrationSystem.h"
#include "Application/Systems/Physics/MovementIntegrationSystem.h"
#include "Application/Systems/Camera/TPSSystem.h"
#include "Application/Systems/Camera/MainCameraSystem.h"
#include "Application/Systems/Camera/CameraProjUpdateSystem.h"
#include "Application/Systems/Camera/RadialBlurSpeedSystem.h"
#include "Application/Systems/Camera/AimTargetSystem.h"
#include "Application/Systems/Transform/CalcMatrixSystem.h"
#include "Application/Systems/Combat/LockOnTargetSystem.h"
#include "Application/Systems/Weapon/MissileSalvoSystem.h"
#include "Application/Systems/Boss/BossMissileSalvoSystem.h"
#include "Application/Systems/Animation/AnimationSystem.h"
#include "Application/Systems/Animation/SkinningSystem.h"
#include "Application/Systems/Animation/CalcNodeSystem.h"
#include "Application/Systems/Transform/FollowAnimationNodeSystem.h"
#include "Application/Systems/Camera/CamSetShaderSystem.h"
#include "Application/Systems/Camera/CameraPipelineSubmitSystem.h"
#include "Application/Systems/Camera/CameraPipelineFixupSystem.h"
#include "Application/Systems/Render/PointLightSystem.h"
#include "Application/Systems/Render/ApplyEmissiveOverrideSystem.h"
#include "Application/Systems/Render/StaticObjectDrawSystem.h"
#include "Application/Systems/Render/DynamicObjectDrawSystem.h"
#include "Application/Systems/Render/AnimationOptionalDraw.h"
#include "Application/Systems/Render/RegisterRayWorldSystem.h"
#include "Application/Systems/Animation/AnimationMatrixFreeSystem.h"
#include "Application/Systems/Render/RegisterPrevWorldMatSystem.h"
#include "Application/Systems/Animation/StateMachineFixupSystem.h"
#include "Application/Systems/Animation/StateMachineCommitSystem.h"
#include "Application/Systems/Animation/PlayerIntentSystem.h"
#include "Application/Systems/Animation/AnimationStateSystem.h"
#include "Application/Systems/Movement/RobotBoostSystem.h"
#include "Application/Systems/Movement/ChargeDashSystem.h"
#include "Application/Systems/Effect/EmitParticlesSystem.h"
#include "Application/Systems/Effect/ParticleEmitSystem.h"
#include "Application/Systems/Effect/ParticleFixupSystem.h"
#include "Application/Systems/Effect/EffectFixupSystem.h"
#include "Application/Systems/Effect/EffectUpdateSystem.h"
#include "Application/Systems/Effect/BoosterEffectSystem.h"
#include "Application/Systems/Effect/EffectDrawSystem.h"
#include "Application/Systems/Transform/UpdateHierarchyDepthSystem.h"
#include "Application/Systems/Transform/CommitHierarchyWorldMatrixSystem.h"
#include "Application/Systems/Render/SkinningRegisterSystem.h"
#include "Application/Systems/Render/RegisterAnimatedRayWorldSystem.h"
#include "Application/Systems/Physics/CapsuleCollisionSystem.h"
#include "Application/Systems/Physics/SphereCollisionSystem.h"
#include "Application/Systems/Input/InputActionSystem.h"
#include "Application/Systems/Weapon/GunShootSystem.h"
#include "Application/Systems/Physics/CollisionEventClearSystem.h"
#include "Application/Systems/Physics/HitDetectSystem.h"
#include "Application/Systems/Combat/ExplodeOnHitSystem.h"
#include "Application/Systems/Core/LifeTimeSystem.h"
#include "Application/Systems/Attachment/AttachmentDispatchSystem.h"
#include "Application/Systems/Weapon/SelfWeaponTriggerSystem.h"
#include "Application/Systems/Effect/ThrusterEffectSystem.h"
#include "Application/Systems/Attachment/AttachmentSlotLinkSystem.h"
#include "Application/Systems/Physics/SyncPhysicsBodySystem.h"
#include "Application/Systems/Animation/AdditivePoseLinkSystem.h"
#include "Application/Systems/Animation/AdditivePoseSystem.h"
#include "Application/Systems/Animation/AdditivePoseFreeSystem.h"
#include "Application/Systems/Enemy/SearchPlayerSystem.h"
#include "Application/Systems/Movement/FaceTargetSystem.h"
#include "Application/Systems/Enemy/EnemyMoveIntentSystem.h"
#include "Application/Systems/Movement/LookAroundSystem.h"
#include "Application/Systems/Enemy/EnemyMovementSystem.h"
#include "Application/Systems/Audio/SoundFixupSystem.h"
#include "Application/Systems/Audio/BoostSoundSystem.h"
#include "Application/Systems/Audio/SpawnSoundSystem.h"
#include "Application/Systems/Audio/SoundFreeSystem.h"
#include "Application/Systems/Physics/PhysicsBodyFreeSystem.h"
#include "Application/Systems/Weapon/GunStateStartSystem.h"
#include "Application/Systems/Combat/HitEventClearSystem.h"
#include "Application/Systems/Combat/DeathEventClearSystem.h"
#include "Application/Systems/Enemy/EnemyShootIntentSystem.h"
#include "Application/Systems/Enemy/CloseCombatIntentSystem.h"
#include "Application/Systems/Boss/BossCombatIntentSystem.h"
#include "Application/Systems/Weapon/HomingSystem.h"
#include "Application/Systems/Audio/HitSoundSystem.h"
#include "Application/Components/Audio/HitSoundComponent.h"
#include "Application/Components/Audio/AudioBehaviorComponent.h"
#include "Application/Systems/Audio/AudioListenerSystem.h"
#include "Application/Systems/Audio/FlyingSoundSystem.h"
#include "Application/Components/Audio/AudioListenerComponent.h"
#include "Application/Components/Audio/FlyingSound.h"
#include "../../InstanceResource/FlyingSoundResource.h"
#include "Application/Components/Combat/HealthComponent.h"
#include "Application/Systems/Combat/HealthFixupSystem.h"
#include "Application/Systems/Combat/HealthSystem.h"
#include "Application/Systems/Combat/DeathStateSystem.h"
#include "Application/Components/Effect/EffectComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Components/Core/LifeTimeComponent.h"
#include "Application/Components/Combat/DeathEffectComponent.h"
#include "Application/Components/Effect/ExplosionComponent.h"
#include "../../InstanceResource/DeathEventResource.h"
#include "../../InstanceResource/WaveAnnounceResource.h"
#include "Application/Systems/Effect/DeathEffectSystem.h"
#include "Application/Systems/Combat/ScoreSystem.h"
#include "Application/Systems/Effect/ExplosionSystem.h"
#include "Application/Systems/Boid/BoidSystem.h"
#include "Application/Systems/Boid/FollowLeaderSystem.h"
#include "Application/Systems/Boid/PlatoonFollowSystem.h"
#include "Application/Systems/Boid/SwarmLookSystem.h"
#include "Application/Systems/Boid/SwarmLeaderMoveSystem.h"
#include "Application/Systems/Boid/BoidWaveSystem.h"
#include "Application/Systems/Boid/SerchGroundSystem.h"
#include "Application/Systems/Boid/BoidGroundEffectSystem.h"
#include "Application/Systems/Effect/DebrisEmitterSystem.h"
#include "Application/Systems/Effect/BallisticSystem.h"
#include "Application/Systems/Boid/BoidContactDamageSystem.h"

// リソース関係
#include "Application/InstanceResource/HierarchyResource.h"
#include "Application/InstanceResource/SingletonEntityResource.h"
#include "Application/InstanceResource/ResourceWaitResource.h"
#include "../../InstanceResource/AdditiveBoneEntry.h"
#include "Application/InstanceResource/HitEventResource.h"
#include "Application/InstanceResource/WormWaveResource.h"
#include "Application/InstanceResource/WormGroundEffectResource.h"
#include "Application/InstanceResource/SwarmContactDamageResource.h"

namespace App::ECS
{
	void RegisterGameTypes(APPWorld& a_world)
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
		a_world.RegisterComponent<FishEyeComponent>("FishEyeComponent");
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
		a_world.RegisterComponent<MoveIntentComponent>("MoveIntentComponent");
		a_world.RegisterComponent<PreviousWorldMatrixComponent>("PreviousWorldMatrixComponent");
		a_world.RegisterComponent<BoostComponent>("BoostComponent");
		a_world.RegisterComponent<AttachmentSlotsComponent>("AttachmentSlotsComponent");
		a_world.RegisterComponent<ParticlesComponent>("ParticlesComponent");
		a_world.RegisterComponent<TPSCameraStateComponent>("TPSCameraStateComponent");
		a_world.RegisterComponent<TPSFollowComponent>("TPSFollowComponent");
		a_world.RegisterComponent<CapsuleColliderComponent>("CapsuleColliderComponent");
		a_world.RegisterComponent<SphereColliderComponent>("SphereColliderComponent");
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
		a_world.RegisterComponent<BoidComponent>("BoidComponent");
		// 群れのボスの先頭(SwarmBossController が指示を出す相手)の印
		a_world.RegisterComponent<BoidLeaderComponent>("BoidLeaderComponent");
		// リーダーに連なる小隊長。一つ前の相手は SwarmBossController が生成時に書き込む
		a_world.RegisterComponent<PlatoonLeaderComponent>("PlatoonLeaderComponent");
		// 自分の周りに出すボイドの設定(数は出す側が決める)
		a_world.RegisterComponent<BoidSpownerComponent>("BoidSpownerComponent");
		// 群れのボスの体を作っているボイドの印。数がそのままボスの体力
		a_world.RegisterComponent<SwarmBossBoidTag>("SwarmBossBoidTag");
		// 上下にレイを打って地面との関係を持つ(今はワームボスのリーダーが使う)
		a_world.RegisterComponent<SerchGroundComponent>("SerchGroundComponent");
		// ワームの体(ボイド)が砂埃を炊く番を待つ時間。付けるのは SwarmBossController
		a_world.RegisterComponent<WarmGroundEffectComponent>("WarmGroundEffectComponent");
		// エフェクトプレハブ : 破片を撒く / 撒かれた破片を放物線で飛ばして着地させる
		a_world.RegisterComponent<DebrisEmitterComponent>("DebrisEmitterComponent");
		a_world.RegisterComponent<BallisticComponent>("BallisticComponent");
		// ワームの体(ボイド)の体当たり。持つのは次に判定するまでの待ち時間だけ
		a_world.RegisterComponent<BoidContactDamageComponent>("BoidContactDamageComponent");
		// 足元の接地判定。書くのは RayCollisionSystem(StateMachineComponent から分けた)
		a_world.RegisterComponent<GroundStateComponent>("GroundStateComponent");
		// ボイドの発光ウェーブの計算途中の値(BoidComponent から分けた)
		a_world.RegisterComponent<BoidWaveStateComponent>("BoidWaveStateComponent");
		// 実行中の発光の差し替え。ModelComponent へ写すのは ApplyEmissiveOverrideSystem
		a_world.RegisterComponent<EmissiveOverrideComponent>("EmissiveOverrideComponent");

		// システム登録
		a_world.RegisterSystem<ModelFixupSystem>();
		a_world.RegisterSystem<GUIDFixupSystem>();
		a_world.RegisterSystem<StateMachineFixupSystem>();
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
		a_world.RegisterSystem<SearchPlayerSystem>();
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
		a_world.RegisterSystem<StateMachineCommitSystem>();
		// コライダーを物理空間(Jolt)へ登録する(静的も動くものも)
		a_world.RegisterSystem<RegisterPhysicsBodySystem>();
		a_world.RegisterSystem<CameraStartSystem>();
		a_world.RegisterSystem<AnimationModelStartSystem>();
		a_world.RegisterSystem<AttachmentNodeLinkSystem>();
		a_world.RegisterSystem<AdditivePoseLinkSystem>();
		// 湧いた瞬間に鳴らす音(エフェクト用)。インスタンスは SoundFixupSystem が先に用意する
		a_world.RegisterSystem<SpawnSoundSystem>();
		a_world.RegisterSystem<CamSetShaderSystem>();
		// 描画構成を持つカメラを全部 GraphicsEngine へ送る(新レンダーグラフ)。
		// メインカメラ1台ぶんを送る CamSetShaderSystem とは別で、こちらは並走する経路
		a_world.RegisterSystem<CameraPipelineSubmitSystem>();
		a_world.RegisterSystem<CameraPipelineFixupSystem>();
		// 点光源の位置と設定値を LightManager へ送る。
		// GPUバッファへ詰め直されるのは描画フェーズの後なので、この帯で間に合う
		a_world.RegisterSystem<PointLightSystem>();
		// 演出側が置いた発光の差し替えを ModelComponent へ写す(描画の直前)
		a_world.RegisterSystem<ApplyEmissiveOverrideSystem>();
		a_world.RegisterSystem<InputMoveSystem>();
		a_world.RegisterSystem<GravitySystem>();
		a_world.RegisterSystem<RotationSystem>();
		// プレイヤーの旋回は「撃っているか」で進行方向/狙い方向を切り替えるので専用システムが持つ
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
		a_world.RegisterSystem<PhysicsBodyFreeSystem>();
		a_world.RegisterSystem<RegisterPrevWorldMatSystem>();
		a_world.RegisterSystem<UpdateHierarchyDepthSystem>();
		a_world.RegisterSystem<CommitHierarchyWorldMatrixSystem>();
		a_world.RegisterSystem<SkinningRegisterSystem>();
		a_world.RegisterSystem<RegisterAnimatedRayWorldSystem>();
		a_world.RegisterSystem<CapsuleCollisionSystem>();
		a_world.RegisterSystem<SphereCollisionSystem>();
		a_world.RegisterSystem<InputActionSystem>();
		a_world.RegisterSystem<GunShootSystem>();
		// 動くコライダーのボディを今の姿勢へ合わせる
		a_world.RegisterSystem<SyncPhysicsBodySystem>();
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
		// 体力を書く HealthSystem との前後は、タスク側の After("HealthSystem") で決めている
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
		a_world.RegisterSystem<BoidSystem>();
		a_world.RegisterSystem<FollowLeaderSystem>();
		// 群れのボスの向き(リーダー/小隊長は進行方向、ボイドは小隊長の向きへ)。
		// 前方を使う PlatoonFollowSystem より前に置く
		a_world.RegisterSystem<SwarmLookSystem>();
		// リーダーの移動入力(SwarmBossController が作る)を目標速度へ
		a_world.RegisterSystem<SwarmLeaderMoveSystem>();
		// 小隊長を一つ前の相手の後ろへ追従させる(目標速度だけ書く)
		a_world.RegisterSystem<PlatoonFollowSystem>();
		// 体を走る発光のウェーブをボイドへ塗る(ウェーブを出すのは SwarmBossController)。
		// 小隊長の向きを使うので SwarmLookSystem より後に置く
		a_world.RegisterSystem<BoidWaveSystem>();
		// 上下にレイを打って地表の高さと地中に居るかを書く(ワームボスのアッパー攻撃が読む)
		a_world.RegisterSystem<SerchGroundSystem>();
		// ワームの体(ボイド)から上下にレイを打ち、地表へ砂埃を炊く(設定は SwarmBossController)
		a_world.RegisterSystem<BoidGroundEffectSystem>();
		// エフェクトプレハブの破片を撒く(破片のハンドルは PostDeserialize で取る)
		a_world.RegisterSystem<DebrisEmitterSystem>();
		// 撒かれた破片を放物線で飛ばし、地面で跳ねて止める
		a_world.RegisterSystem<BallisticSystem>();
		// ワームの体(ボイド)がプレイヤーに触れたらダメージを積む(減らすのは HealthSystem)
		a_world.RegisterSystem<BoidContactDamageSystem>();

		// インスタンスデータの登録
		a_world.AddResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

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
		a_world.AddResource<WormWaveResource>();
		// ワームの体が炊く砂埃の設定(SwarmBossController が書き、BoidGroundEffectSystem が読む)
		a_world.AddResource<WormGroundEffectResource>();
		// ワームの体当たりの設定とプレイヤーの形(SwarmBossController が書き、BoidContactDamageSystem が読む)
		a_world.AddResource<SwarmContactDamageResource>();

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

		// 同時に走るウェーブは数本(SwarmBossController の Max Wave)
		a_world.GetResource<WormWaveResource>().Reserve(16);
	}
}
