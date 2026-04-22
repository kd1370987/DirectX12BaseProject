#pragma once

enum class Layer : uint32_t
{
	None			= 0,
	StaticObject	= 1 << 0,
	DiynamicObject	= 1 << 1,
	Trigger			= 1 << 2,
};

struct ColliderComponent
{
	Layer layer = Layer::StaticObject;		// 自分が属するレイヤー
	Layer collideLayer = Layer::None;		// 衝突したいレイヤー
	Engine::ECS::Flg isPhysical = 1;		// 物理解決するかどうか(衝突時にイベントだけほしいとか)
	
	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const ColliderComponent*>(a_ptr);
		a_json["layer"] = static_cast<uint32_t>(_comp->layer);
		a_json["collideLayer"] = static_cast<uint32_t>(_comp->collideLayer);
		a_json["isPhysical"] = _comp->isPhysical;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<ColliderComponent*>(a_ptr);
		_comp->layer = static_cast<Layer>(JSONHelper::GetValue<uint32_t>("layer", a_json, 0));
		_comp->collideLayer = static_cast<Layer>(JSONHelper::GetValue<uint32_t>("collideLayer",a_json,0));
		_comp->isPhysical = static_cast<Engine::ECS::Flg>(a_json["isPhysical"].get<uint32_t>());
		_comp->isPhysical = static_cast<Engine::ECS::Flg>(JSONHelper::GetValue<uint32_t>("isPhysical", a_json, 1));
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		ColliderComponent& _comp = Engine::Editor::GetValue<ColliderComponent>(a_data);
		Editor::DrawEnumCombo("MyLayer", _comp.layer);
		Editor::DrawEnumFlagsCombo("HItLayer", _comp.collideLayer);
		bool _is = _comp.isPhysical != 0;
		if (ImGui::Checkbox("IsPhysical", &_is))
		{
			_comp.isPhysical = _is ? 1u : 0u;
		}
	}

};

inline Layer operator|(Layer a, Layer b)
{
	return static_cast<Layer>(
		static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
		);
}

inline Layer operator&(Layer a, Layer b)
{
	return static_cast<Layer>(
		static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
		);
}

inline Layer& operator|=(Layer& a, Layer b)
{
	a = a | b;
	return a;
}

inline bool HasLayer(Layer value, Layer test)
{
	return (value & test) != Layer::None;
}

// 形状情報、質量。動く、動かない。衝突時の挙動などは持たせない。
