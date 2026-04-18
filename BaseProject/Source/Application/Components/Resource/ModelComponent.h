#pragma once

#include "../../../Engine/Resource/Manager/ModelManager/ModelManager.h"

struct ModelComponent
{
	DirectX::XMFLOAT4 colorScale = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 emissiveScale = { 1.0f,1.0f,1.0f };

	Engine::Resource::Handle<Engine::Resource::Model> handle = {};

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(ModelComponent,emissiveScale),
				[](void* a_data)
				{
				// 現在の表示
				ImGui::Text("EmissiveScale");
				ImGui::ColorPicker4("Color",(float*)a_data);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(ModelComponent,colorScale),
				[](void* a_data)
				{
					// 現在の表示
					ImGui::Text("ColorScale");
					

					ImGui::ColorPicker4("Color",(float*)a_data);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(ModelComponent,handle),
				[](void* a_data)
				{
					auto& _handle = *reinterpret_cast<Resource::Handle<Resource::Model>*>(a_data);
					auto* _model = Resource::ModelManager::Instnace().RefModel(_handle);
					// 現在の表示
					ImGui::Text("Model : %s",_model->name.c_str());

					auto& _models = Resource::ModelManager::Instnace().GetAllModel();

					// 選択UI
					if (ImGui::BeginCombo("Change Model", "Select..."))
					{
						for (auto& _model : _models)
						{
							// 選択中のモデルだったらフラグを立てる
							auto _thisHandle = Resource::ModelManager::Instnace().GetHandle(_model.data.name);
							bool _selected = (_handle == _thisHandle);

							// 選択欄
							if (ImGui::Selectable(_model.data.name.c_str(), _selected))
							{
								_handle = _thisHandle;
							}
						}
						ImGui::EndCombo();
					}
				}
			}
		};
	}
};