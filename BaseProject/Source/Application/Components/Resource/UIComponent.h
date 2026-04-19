#pragma once

#include "../../../Engine/Resource/Manager/TextureManager/TextureManager.h"

struct UIComponent
{
	// テクスチャID
	Engine::Resource::Handle<Engine::Resource::Texture> texHandle = {};

	// UVオフセットとタイル
	DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// 色
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(UIComponent,texHandle),
				[](void* a_data)
				{
					auto& _handle = *reinterpret_cast<Resource::Handle<Resource::Texture>*>(a_data);
					auto& _tex = Resource::TextureManager::Instance().RefTexture(_handle);
					// 現在の表示
					ImGui::Text("Tex : %s",_tex.GetName().c_str());

					auto& _textures = Resource::TextureManager::Instance().GetAllTex();

					// 選択UI
					if (ImGui::BeginCombo("Change Texture", "Select..."))
					{
						for (auto& _tex : _textures)
						{
							// 選択中のモデルだったらフラグを立てる
							auto _thisHandle = Resource::TextureManager::Instance().GetHandle(_tex.GetName());
							bool _selected = (_handle == _thisHandle);

							// 選択欄
							if (ImGui::Selectable(_tex.GetName().c_str(), _selected))
							{
								_handle = _thisHandle;
							}
						}
						ImGui::EndCombo();
					}
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(UIComponent,uvOffsetTiling),
				[](void* a_data)
				{
					DirectX::XMFLOAT4& _uvOffsetTie = Editor::GetValue<DirectX::XMFLOAT4>(a_data);

					ImGui::DragFloat2("UVOffset",&_uvOffsetTie.x,0.1f);

					ImGui::DragFloat2("UVTile",&_uvOffsetTie.z,0.1f);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(UIComponent,color),
				[](void* a_data)
				{
					ImGui::Text("ColorScale");
					ImGui::ColorPicker4("Color", (float*)a_data);
				}
			}
		};
	}
};