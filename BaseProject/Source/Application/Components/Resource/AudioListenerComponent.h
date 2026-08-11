#pragma once

#include "../../../Engine/ECS/World/World.h"

//==========================================================================================
// AudioListenerComponent
//
// 3Dサウンドの「聞き手」。付けたエンティティの位置・向きを、
// AudioListenerSystem が毎フレーム AudioManager へ送る。
//
// プレイヤーに付ける想定。付いていない間 AudioManager のリスナーは初期値
// (原点・+Z向き)のままなので、3D音の定位が原点基準になってしまう。
//
// 複数付けても壊れはしないが、最後に送ったものが残るだけなので1つにすること。
//==========================================================================================
struct AudioListenerComponent
{
	// 耳の位置。エンティティ原点からのローカルオフセット(足元基準のモデルで頭の高さへ上げる等)
	DirectX::XMFLOAT3 posOffset = { 0.0f, 0.0f, 0.0f };

	// 速度も送るか。ドップラーを掛けたくない場合は false
	bool useVelocity = true;

	// ---- ランタイム(保存しない) ----
	DirectX::XMFLOAT3 prevPos = { 0.0f, 0.0f, 0.0f };	// 速度を出すための前フレーム位置
	bool hasPrevPos = false;							// 1フレーム目は速度を出せない
};

template<>
struct Engine::ECS::ComponentTraits<AudioListenerComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AudioListenerComponent& _comp = Engine::Editor::GetValue<AudioListenerComponent>(a_pData);
		a_ar.Field("posOffset", _comp.posOffset);
		a_ar.Field("useVelocity", _comp.useVelocity);
	}

	static void Edit(CompEditContext& a_context)
	{
		AudioListenerComponent& _comp = Engine::Editor::GetValue<AudioListenerComponent>(a_context.pData);

		ImGui::DragFloat3("PosOffset", &_comp.posOffset.x, 0.05f);
		ImGui::Checkbox("UseVelocity (Doppler)", &_comp.useVelocity);
	}
};
