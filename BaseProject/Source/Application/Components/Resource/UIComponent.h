#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Loader/Texture/TextureLoader.h"

struct UIComponent
{
	// テクスチャID
	// ランタイム用
	Engine::Handle<Engine::Resource::Texture> texHandle = {};
	Engine::GUID texGUID = {};

	// UVオフセットとタイル
	DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// 色
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
};

template<>
struct Engine::ECS::ComponentTraits<UIComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		UIComponent& _comp = Engine::Editor::GetValue<UIComponent>(a_pData);
		a_ar.Field("uvOffsetTiling", _comp.uvOffsetTiling);
		a_ar.Field("color", _comp.color);
		a_ar.Field("texGUID", _comp.texGUID);

	}

	static void Edit(void* a_pData)
	{
		// 参照
		using namespace Engine;
		UIComponent& _comp = Engine::Editor::GetValue<UIComponent>(a_pData);
		auto* _pTex = Resource::ResourceManager::Instance().Ref(_comp.texHandle);

		// UV関連の設定
		ImGui::DragFloat2("UVOffset", &_comp.uvOffsetTiling.x, 0.1f);
		ImGui::DragFloat2("UVTile", &_comp.uvOffsetTiling.z, 0.1f);


		// 現在の表示
		ImGui::Text("Tex : %s", _pTex->GetName().c_str());
		ImGui::Text("%s", _comp.texGUID.String().c_str());

		const auto& _textures = Resource::ResourceManager::Instance().GetPool<Resource::Texture>();

		// 選択UI
		if (ImGui::BeginCombo("Change Texture", "Select..."))
		{
			//for (const auto& [_guid,_handle] : Resource::TextureLoader::GetAllCache())
			for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("Texture"))
			{
				// 選択中のモデルだったらフラグを立てる
				bool _selected = (_comp.texGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					_comp.texHandle = Resource::ResourceManager::Instance().Load<Resource::Texture>(_prop.guid);
					_comp.texGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Text("ColorScale");
		ImGui::ColorPicker4("ColorScale", &_comp.color.x);
	}
};