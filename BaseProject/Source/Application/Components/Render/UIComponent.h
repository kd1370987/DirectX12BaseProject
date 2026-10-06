#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/EditorField/EditorField.inl"

namespace App::Component
{
	struct UIComponent
	{
		// テクスチャID
		// ランタイム用
		Engine::Handle<Engine::Resource::Texture> texHandle = {};
		Core::GUID texGUID = {};

		// UVオフセットとタイル
		Math::Vector4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
		// 色
		Math::Color color = { 1.0f,1.0f,1.0f,1.0f };
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::UIComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::UIComponent& _comp = Engine::EditorField::RefValue<App::Component::UIComponent>(a_pData);
		a_ar.Field("uvOffsetTiling", _comp.uvOffsetTiling);
		a_ar.Field("color", _comp.color);
		a_ar.Field("texGUID", _comp.texGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		// 参照
		using namespace Engine;
		App::Component::UIComponent& _comp = Engine::EditorField::RefValue<App::Component::UIComponent>(a_context.pData);

		// UV関連の設定 : uvOffsetTiling は xy がオフセット、zw がタイリング
		Math::Vector2 _uvOffset(_comp.uvOffsetTiling.x, _comp.uvOffsetTiling.y);
		if (Engine::EditorField::Field("UVOffset", _uvOffset, 0.1f))
		{
			_comp.uvOffsetTiling.x = _uvOffset.x;
			_comp.uvOffsetTiling.y = _uvOffset.y;
		}
		Math::Vector2 _uvTile(_comp.uvOffsetTiling.z, _comp.uvOffsetTiling.w);
		if (Engine::EditorField::Field("UVTile", _uvTile, 0.1f))
		{
			_comp.uvOffsetTiling.z = _uvTile.x;
			_comp.uvOffsetTiling.w = _uvTile.y;
		}

		// テクスチャの選択(現在の表示もヘルパー側で行う)
		Engine::EditorField::AssetField<Resource::Texture>(
			*a_context.pWorld->RefEngineServices(),
			"Texture",
			"Texture",
			_comp.texGUID,
			_comp.texHandle
		);

		Engine::EditorField::ColorPicker("ColorScale", _comp.color);
	}
};