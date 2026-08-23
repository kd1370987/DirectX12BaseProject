#pragma once

//==========================================================================================
// 当たり判定のレイヤー
//
// layer        : 自分がどれか1つ(単一選択)
// collideLayer : 当たりに行きたい相手(複数選択)
//
// 弾は「撃った側」でレイヤーを分けている。
// プレイヤーも敵もまったく同じ弾/ミサイルのプレハブを撃つので、
// どちら側のものかをプレハブへ書いておけない。
// 発射のたびに ProjectileSpawn が撃った本体を見て入れる。
//
// 分けている理由は弾同士の相殺。以前は弾もまとめて DiynamicObject だったため、
// 自分が撃った弾とミサイルがぶつかって発射直後に消えていた
// (斉射のように同じ場所から続けて出るものでは必ず起きる)。
// 相手側の弾のレイヤーだけを collideLayer へ入れておけば、
// 敵のミサイルは今までどおり撃ち落とせて、自分の弾同士は素通りする。
//==========================================================================================
enum class Layer : uint32_t
{
	None			= 0,
	StaticObject	= 1 << 0,
	DiynamicObject	= 1 << 1,
	Trigger			= 1 << 2,

	PlayerProjectile	= 1 << 3,	// プレイヤー側が撃った弾・ミサイル
	EnemyProjectile		= 1 << 4,	// 敵側が撃った弾・ミサイル
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

//------------------------------------------------------------------------------------------
// 毎フレーム位置が変わる側のレイヤーか(=動的ワールドへ毎フレーム submit する側か)
//
// 弾を撃った側で分けたことで、動くものが DiynamicObject だけではなくなった。
// 静的か動的かを見るところは必ずここを通すこと。
// == Layer::DiynamicObject で見たままにしておくと、弾が静的ワールドへ登録され、
// 撃った瞬間の場所に当たり判定が置き去りになる(絵だけ飛んでいく)。
//------------------------------------------------------------------------------------------
inline bool IsDynamicLayer(Layer a_layer)
{
	return HasLayer(a_layer,
		Layer::DiynamicObject | Layer::PlayerProjectile | Layer::EnemyProjectile);
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
		Editor::EditorHelper::DrawEnumCombo("MyLayer", _comp.layer);
		Editor::EditorHelper::DrawEnumFlagsCombo("HItLayer", _comp.collideLayer);

		// 物理解決
		bool _is = _comp.isPhysical != 0;
		if (ImGui::Checkbox("IsPhysical", &_is))
		{
			_comp.isPhysical = _is ? 1u : 0u;
		}

		// シェープタイプ
		Editor::EditorHelper::DrawEnumCombo("ShapeType",_comp.shapeType.type);
	}
};