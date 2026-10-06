#pragma once

namespace App::Component
{
	//==========================================================================================
	// CameraFocusTargetComponent
	//
	// カメラ本体ではなく、カメラがフォーカスする対象につけるコンポーネント。
	//
	// offsetPos は「カメラ空間(TPSカメラのオービット基準)」のオフセット。
	// 左手系なので +X が画面右、+Y が画面上、+Z が視線の奥。
	// TPSカメラはこの点を注視するので、機体は画面上でオフセットと逆側へずれる。
	//   例) x,y を正 → 注視点が機体の右上 → 機体は画面の左下寄りに映る。
	//
	// 機体の姿勢基準にしてはいけない。プレイヤーの胴体は狙点/進行方向へ
	// 遅れて向くので視線角と一致せず、旋回や横移動のたびに構図が振られる。
	//==========================================================================================
	struct CameraFocusTargetComponent
	{
		Math::Vector3 offsetPos;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::CameraFocusTargetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::CameraFocusTargetComponent& _comp = Engine::EditorField::GetValue<App::Component::CameraFocusTargetComponent>(a_pData);
		a_ar.Field("offsetPos", _comp.offsetPos);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::CameraFocusTargetComponent& _comp = Engine::EditorField::GetValue<App::Component::CameraFocusTargetComponent>(a_context.pData);
		Engine::EditorField::Field("OffsetPos", _comp.offsetPos, 0.01f);
	}
};