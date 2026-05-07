#pragma once

//#include "../../../Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Loader/Texture/TextureLoader.h"

struct UIComponent
{
	// テクスチャID
	// ランタイム用
	Engine::Resource::Handle<Engine::Resource::Texture> texHandle = {};
	Engine::GUID texGUID = {};

	// UVオフセットとタイル
	DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// 色
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const UIComponent*>(a_ptr);
		a_json["uvOffsetTiling"] = {_comp->uvOffsetTiling.x,_comp->uvOffsetTiling.y ,_comp->uvOffsetTiling.z ,_comp->uvOffsetTiling.w};
		a_json["color"] = { _comp->color.x,_comp->color.y, _comp->color.z, _comp->color.w };
		//a_json["texGUID"] = Engine::GUID::ToString(_comp->texGUID);
		a_json["texGUID"] = _comp->texGUID.String();
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<UIComponent*>(a_ptr);
		//_comp->texGUID = Engine::GUID::FromString(a_json["texGUID"].get<std::string>());
		_comp->texGUID.FromString(a_json["texGUID"].get<std::string>());
	}

	static void Edit(void* a_data)
	{
		//using namespace Engine;
		//UIComponent& _comp = Engine::Editor::GetValue<UIComponent>(a_data);

		//ImGui::DragFloat2("UVOffset", &_comp.uvOffsetTiling.x, 0.1f);
		//ImGui::DragFloat2("UVTile", &_comp.uvOffsetTiling.z, 0.1f);
		//
		////auto& _tex = Resource::TextureManager::Instance().RefTexture(_comp.texHandle);
		//auto* _pTex = Resource::ResourceManager::Instance().Ref(_comp.texHandle);
		//// 現在の表示
		//ImGui::Text("Tex : %s", _pTex->GetName().c_str());

		//const auto& _textures = Resource::ResourceManager::Instance().GetPool<Resource::Texture>();

		//// 選択UI
		//if (ImGui::BeginCombo("Change Texture", "Select..."))
		//{
		//	for (const auto& _tex : _textures)
		//	{
		//		// 選択中のモデルだったらフラグを立てる
		//		auto _thisHandle = Resource::TextureManager::Instance().GetHandle(_tex.GetName());
		//		bool _selected = (_comp.texHandle == _thisHandle);

		//		// 選択欄
		//		if (ImGui::Selectable(_tex.GetName().c_str(), _selected))
		//		{
		//			_comp.texHandle = _thisHandle;
		//		}
		//	}
		//	ImGui::EndCombo();
		//}

		//ImGui::Text("ColorScale");
		//ImGui::ColorPicker4("Color", &_comp.color.x);
	}
};