#pragma once

//==========================================================================================
// LookAngleComponent
//
// 「どの方向を見ているか」を Yaw / Pitch(度)で保持する汎用コンポーネント。
// プレイヤー専用ではないので、敵など視線を持つキャラクターにも同じ意味で付けられる。
//
// ・プレイヤーは InputMoveSystem がマウス/スティック入力で更新する。
// ・TPSSystem(カメラ姿勢)、AdditivePoseSystem(上体の狙い)、
//   CharacterMovementSystem(移動の基準軸)がこの角度を読む。
//
// 体(LocalTransform)の向きをこの角度へ合わせるのは RotationSystem。
// プレイヤーだけは ActionState を見て「進行方向 / 狙い方向」を切り替える
// LockOnRotationSystem が担当するため、RotationSystem からは除外される。
//
// ・体の向きに使うのは既定では Yaw だけ。人型は上体だけで狙う(上下は
//   AdditivePoseSystem が上体のボーンへ足す)ので、Pitch で機体ごと傾けない。
//   空を泳ぐもの(群れのボスなど)は体ごと上下を向かせたいので、
//   isApplyPitchToBody を立てると RotationSystem が Pitch も入れて回す。
//==========================================================================================
struct LookAngleComponent
{
	float Yaw = 0.0f;		// 水平角(度)
	float Pitch = 0.0f;		// 仰角(度)

	float maxPitch = 80.0f;	// Pitch の可動域(±)

	// 体の向きにも Pitch を入れるか(RotationSystem が見る)。
	// 人型は false のまま。上下に機体ごと向くものだけ立てる
	bool isApplyPitchToBody = false;
};

//==========================================================================================
// この角度が向いている方向(単位ベクトル)
//
// 左手系 +Z 前方 / Pitch は上向きが正(BossCombatIntentSystem と同じ規約)。
// 「前を向いている向き」を欲しがる側が毎回同じ式を書かなくて済むように置いてある。
//
// ※ 体の向き(LocalTransform.quat)は RotationSystem が Yaw だけを使って作る。
//    Pitch まで入れたこの方向と体の向きは一致しないので、見た目に合わせたいときは Yaw だけ使うこと
//==========================================================================================
inline Math::Vector3 MakeLookForward(const LookAngleComponent& a_look)
{
	const float _yaw   = DirectX::XMConvertToRadians(a_look.Yaw);
	const float _pitch = DirectX::XMConvertToRadians(a_look.Pitch);

	const float _cosPitch = std::cos(_pitch);

	return Math::Vector3(
		std::sin(_yaw) * _cosPitch,
		std::sin(_pitch),
		std::cos(_yaw) * _cosPitch);
}

//==========================================================================================
// 方向から Yaw / Pitch(度)を作る。長さが無いときは false(角度は触らない)
//==========================================================================================
inline bool MakeLookAngleFromDir(const Math::Vector3& a_dir, float& a_outYawDeg, float& a_outPitchDeg)
{
	Math::Vector3 _dir = a_dir;
	if (_dir.LengthSquared() < 1e-8f) return false;

	_dir.Normalize();

	a_outYawDeg   = DirectX::XMConvertToDegrees(std::atan2(_dir.x, _dir.z));
	a_outPitchDeg = DirectX::XMConvertToDegrees(std::asin(std::clamp(_dir.y, -1.0f, 1.0f)));
	return true;
}

template<>
struct Engine::ECS::ComponentTraits<LookAngleComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LookAngleComponent& _comp = Engine::Editor::GetValue<LookAngleComponent>(a_pData);
		a_ar.Field("Yaw", _comp.Yaw);
		a_ar.Field("Pith", _comp.Pitch);		// 既存データとの互換のためキー名はそのまま
		a_ar.Field("maxPitch", _comp.maxPitch);
		a_ar.Field("isApplyPitchToBody", _comp.isApplyPitchToBody);
	}

	static void Edit(CompEditContext& a_context)
	{
		LookAngleComponent& _comp = Engine::Editor::GetValue<LookAngleComponent>(a_context.pData);
		Engine::Editor::Field("Yaw", _comp.Yaw, 0.1f);
		Engine::Editor::Field("Pith", _comp.Pitch, 0.1f);
		Engine::Editor::Line();
		Engine::Editor::Field("MaxPitch", _comp.maxPitch, 0.1f);
		Engine::Editor::Field("ApplyPitchToBody", _comp.isApplyPitchToBody);
		Engine::Editor::Tooltip("(body tilts up/down. humanoids : off)");
	}
};
