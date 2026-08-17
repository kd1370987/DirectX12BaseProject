#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Editor/Helper/EditorHelper.inl"

struct UIComponent
{
	// テクスチャID
	// ランタイム用
	Engine::Handle<Engine::Resource::Texture> texHandle = {};
	Engine::GUID texGUID = {};

	// UVオフセットとタイル
	Math::Vector4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// 色
	Math::Color color = { 1.0f,1.0f,1.0f,1.0f };
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

	static void Edit(CompEditContext& a_context)
	{
		// 参照
		using namespace Engine;
		UIComponent& _comp = Engine::Editor::GetValue<UIComponent>(a_context.pData);

		// UV関連の設定
		ImGui::DragFloat2("UVOffset", &_comp.uvOffsetTiling.x, 0.1f);
		ImGui::DragFloat2("UVTile", &_comp.uvOffsetTiling.z, 0.1f);

		// テクスチャの選択(現在の表示もヘルパー側で行う)
		Editor::EditorHelper::DrawAssetSelectCombo<Resource::Texture>(
			"Change Texture",
			"Texture",
			_comp.texGUID,
			_comp.texHandle
		);

		ImGui::Text("ColorScale");
		ImGui::ColorPicker4("ColorScale", _comp.color.Data());
	}
};