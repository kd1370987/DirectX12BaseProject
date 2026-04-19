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
	
	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(ColliderComponent,layer),
				[](void* a_data)
				{
					Layer& _layer = Editor::GetValue<Layer>(a_data);
					Editor::DrawEnumCombo("MyLayer",_layer);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(ColliderComponent,collideLayer),
				[](void* a_data)
				{
					Layer& _layer = Editor::GetValue<Layer>(a_data);
					Editor::DrawEnumFlagsCombo("HItLayer",_layer);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(ColliderComponent,isPhysical),
				[](void* a_data)
				{
					ECS::Flg& _isPhysical = Editor::GetValue<ECS::Flg>(a_data);
					bool _is = _isPhysical != 0;
					if (ImGui::Checkbox("IsPhysical", &_is))
					{
						_isPhysical = _is ? 1u : 0u;
					}
				}
			}
		};
	};
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
