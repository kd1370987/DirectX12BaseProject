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
//==========================================================================================
struct LookAngleComponent
{
	float Yaw = 0.0f;		// 水平角(度)
	float Pitch = 0.0f;		// 仰角(度)

	float maxPitch = 80.0f;	// Pitch の可動域(±)
};

template<>
struct Engine::ECS::ComponentTraits<LookAngleComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LookAngleComponent& _comp = Engine::Editor::GetValue<LookAngleComponent>(a_pData);
		a_ar.Field("Yaw", _comp.Yaw);
		a_ar.Field("Pith", _comp.Pitch);		// 既存データとの互換のためキー名はそのまま
		a_ar.Field("maxPitch", _comp.maxPitch);
	}

	static void Edit(CompEditContext& a_context)
	{
		LookAngleComponent& _comp = Engine::Editor::GetValue<LookAngleComponent>(a_context.pData);
		ImGui::DragFloat("Yaw", &_comp.Yaw, 0.1f);
		ImGui::DragFloat("Pith", &_comp.Pitch, 0.1f);
		ImGui::Separator();
		ImGui::DragFloat("MaxPitch", &_comp.maxPitch, 0.1f);
	}
};
