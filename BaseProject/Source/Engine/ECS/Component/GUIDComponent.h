#pragma once
namespace Engine::ECS
{
	//==========================================================================================
	// エンティティの永続ID
	//
	// シーンの保存(BaseScene::Archive)とプレハブの展開(Prefab)が、保存・参照の張り替えの
	// 目印に使う。エンジンの仕組みが前提にしているので Engine に置く(登録はゲーム側の
	// WorldTypeRegister。保存キーは "GUIDComponent")
	//==========================================================================================
	struct GUIDComponent
	{
		Core::GUID guid = {};
	};
}

template<>
struct Engine::ECS::ComponentTraits<Engine::ECS::GUIDComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		Engine::ECS::GUIDComponent& _comp = Engine::EditorField::GetValue<Engine::ECS::GUIDComponent>(a_pData);
		a_ar.Field("guid", _comp.guid);
	}

	static void Edit(CompEditContext& a_context)
	{
		Engine::ECS::GUIDComponent& _comp = Engine::EditorField::GetValue<Engine::ECS::GUIDComponent>(a_context.pData);
		Engine::EditorField::Text("%s", _comp.guid.String().c_str());
	}
};