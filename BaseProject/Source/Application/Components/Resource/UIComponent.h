#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Editor/Helper/EditorField.inl"

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

		// UV関連の設定 : uvOffsetTiling は xy がオフセット、zw がタイリング
		Math::Vector2 _uvOffset(_comp.uvOffsetTiling.x, _comp.uvOffsetTiling.y);
		if (Engine::Editor::Field("UVOffset", _uvOffset, 0.1f))
		{
			_comp.uvOffsetTiling.x = _uvOffset.x;
			_comp.uvOffsetTiling.y = _uvOffset.y;
		}
		Math::Vector2 _uvTile(_comp.uvOffsetTiling.z, _comp.uvOffsetTiling.w);
		if (Engine::Editor::Field("UVTile", _uvTile, 0.1f))
		{
			_comp.uvOffsetTiling.z = _uvTile.x;
			_comp.uvOffsetTiling.w = _uvTile.y;
		}

		// テクスチャの選択(現在の表示もヘルパー側で行う)
		Engine::Editor::AssetField<Resource::Texture>(
			*a_context.pWorld->RefEngineServices(),
			"Texture",
			"Texture",
			_comp.texGUID,
			_comp.texHandle
		);

		Engine::Editor::ColorPicker("ColorScale", _comp.color);
	}
};