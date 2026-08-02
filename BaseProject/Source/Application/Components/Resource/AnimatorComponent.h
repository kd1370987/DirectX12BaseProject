#pragma once

#include "../../../Engine/ECS/World/World.h"
#include "ModelComponent.h"

struct AnimatorComponent
{
	uint32_t clipID = 0;
	Engine::Handle<Engine::Resource::AnimationData> animHandle;
	float time = 0.0f;
	float speed = 1.0f;

	// 加算ポーズの効き。現在のステートの値が毎フレーム流し込まれる。
	float additiveWeight = 1.0f;

	Engine::ECS::Flg isLoop = 0;

	// レイトレをする際にインスタンスを確保する
	Engine::Handle<Engine::Raytracing::DynamicRaytracingData> dynamicInstanceHandle = {};
};

template<>
struct Engine::ECS::ComponentTraits<AnimatorComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("isLoop", _comp.isLoop);
	}

	static void Edit(CompEditContext& a_context)
	{
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_context.pData);
		ImGui::Text("Handle : idx = %d,  gen = %d", (int)_comp.animHandle.GetIndex(), (int)_comp.animHandle.GetGeneration());
		ImGui::InputScalar("clipID", ImGuiDataType_U32, &_comp.clipID);
		ImGui::Text("Time : %f", &_comp.time);

		ImGui::DragFloat("Speed", &_comp.speed);

		ECS::Flg& _isLoop = _comp.isLoop;
		bool _value = _isLoop != 0;
		if (ImGui::Checkbox("IsLoop", &_value))
		{
			_isLoop = _value ? 1u : 0u;
		}

		Engine::Editor::Helper::DrawHandle(_comp.dynamicInstanceHandle);

		// プレハブ編集では実体が無い(entity は INVALID)。
		// 無効IDでエンティティ参照するとレンジ外になるので、実体があるときだけ辿る。
		if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY &&
			a_context.pWorld->HasComponent<ModelComponent>(a_context.entity))
		{
			auto* _refData = a_context.pWorld->RefData<ModelComponent>(a_context.entity);
			if (!_refData) return;

			auto* _pModel = Resource::ResourceManager::Instance().Get(_refData->handle);
			if (!_pModel) return;

			// モデル内のアニメーションコンボ
			Engine::Editor::EditorHelper::DrawModelAnimationCombo("Animation", _pModel, _comp.animHandle);
		}
	}
};