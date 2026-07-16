#pragma once

#include "../../../Engine/ECS/World/World.h"
#include "ModelComponent.h"

struct AnimatorComponent
{
	uint32_t clipID = 0;
	Engine::Handle<Engine::Resource::AnimationData> animHandle;
	float time = 0.0f;
	float speed = 1.0f;

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

		if (a_context.pWorld->HasComponent<ModelComponent>(a_context.entity))
		{
			auto* _refData = a_context.pWorld->RefData<ModelComponent>(a_context.entity);
			if (!_refData) return;

			auto* _pModel = Resource::ResourceManager::Instance().Get(_refData->handle);
			if (!_pModel) return;

			// モデル内のアニメーションコンボ
			bool _isChanged = false;

			// 現在の情報
			auto* _pCurrentAnim = Resource::ResourceManager::Instance().Get(_comp.animHandle);
			if (_pCurrentAnim)
			{
				ImGui::Text("%s",_pCurrentAnim->name.c_str());
			}
			else
			{
				ImGui::Text("No selected");
			}
			
			// 選択UI
			if (ImGui::BeginCombo("Animation", "Selected..."))
			{
				for (auto& _handle : _pModel->GetAnimationHandles())
				{
					bool _isSelected = (_handle == _comp.animHandle);

					auto* _pAnim = Resource::ResourceManager::Instance().Get(_handle);
					if (!_pAnim) continue;

					// 選択欄
					if (ImGui::Selectable(_pAnim->name.c_str(), _isSelected))
					{
						// ハンドルとGUIDを更新
						_comp.animHandle = _handle;
						_isChanged = true;
					}

					// コンボボックスを開いた際、現在の選択アイテムまで自動スクロールする
					if (_isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

		
		}
	}
};