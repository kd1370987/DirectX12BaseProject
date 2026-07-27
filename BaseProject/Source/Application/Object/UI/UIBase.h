#pragma once

#include "../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	class UIBase : public Engine::GameObject::BaseObject
	{
	public:

		// 解放処理
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// 描画するUIの構成テクスチャ
		std::vector<Engine::Handle<Engine::Resource::Texture>> m_textureHandles = {};

	};
}