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

	Engine::Collision::ColliderShape shapeType;

	// コリジョンワールドに登録されているハンドル
	Engine::Handle<Engine::Collision::CollisionInstance> collWorldHandle = {};
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
template<>
struct Engine::ECS::ComponentTraits<ColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ColliderComponent& _comp = Engine::Editor::GetValue<ColliderComponent>(a_pData);
		a_ar.Field("layer", _comp.layer);
		a_ar.Field("collideLayer", _comp.collideLayer);
		a_ar.Field("isPhysical", _comp.isPhysical);

		a_ar.Field("shapeType",_comp.shapeType.type);

		//switch (_comp.shapeType.type)
		//{
		//case Collision::EShapeType::Sphere :
		//	a_ar.Field("sphereRadius",_comp.shapeType.sphere.radius);
		//	break;
		//case Collision::EShapeType::Box:
		//	a_ar.Field("extents",_comp.shapeType.box.extents);
		//	break;
		//case Collision::EShapeType::Capsule:
		//	a_ar.Field("capsuleRadius",_comp.shapeType.capsule.radius);
		//	a_ar.Field("capsuleHeight",_comp.shapeType.capsule.height);
		//	break;
		//case Collision::EShapeType::Mesh:
		//	break;
		//default:
		//	break;
		//}
	}

	static void Edit(CompEditContext& a_context)
	{
		// コンポーネント取得
		using namespace Engine;
		ColliderComponent& _comp = Engine::Editor::GetValue<ColliderComponent>(a_context.pData);

		// レイヤー選択
		Editor::DrawEnumCombo("MyLayer", _comp.layer);
		Editor::DrawEnumFlagsCombo("HItLayer", _comp.collideLayer);

		// 物理解決
		bool _is = _comp.isPhysical != 0;
		if (ImGui::Checkbox("IsPhysical", &_is))
		{
			_comp.isPhysical = _is ? 1u : 0u;
		}

		// シェープタイプ
		Editor::DrawEnumCombo("ShapeType",_comp.shapeType.type);
	}
};