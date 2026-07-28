#pragma once

#include "../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	struct UIElement
	{
		Engine::ResourceRef<Engine::Resource::Texture> texHandle = {};
		DXSM::Vector2 screenPos = {};
		float rotation = 0.0f;

		DXSM::Vector2 size = {};
		float scale = 0.0f;

		DXSM::Color color = {};
	};

	class UIBase : public Engine::GameObject::BaseObject
	{
	public:

		// 解放処理
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// 描画するUIの構成テクスチャ
		std::vector<UIElement> m_elemets = {};

	};
}