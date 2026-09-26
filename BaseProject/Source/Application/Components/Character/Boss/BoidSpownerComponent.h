#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorField.h"

//==========================================================================================
// BoidSpownerComponent
//
// 自分の周りに出すボイドの設定。小隊長のプレハブに持たせる。
//
// ・何体出すかはここでは持たない。群れ全体の数を決めるのは出す側
//   (SwarmBossController が最大ボイド数を小隊長の数で割って振り分ける)。
//   ここが持つのは「どのボイドを・どれだけの広さにばらまくか」だけ。
// ・prefab は初回生成時に解決するランタイム値。参照を取るのは生成する側
//   (App::Utility の Acquire 経由)で、返すのはこのコンポーネントの解放フック。
//==========================================================================================
struct BoidSpownerComponent
{
	Engine::GUID boidPrefabGUID = {};					// 出すボイド(保存される)
	Engine::Handle<Engine::Resource::Prefab> prefab;	// ランタイム用(初回生成時に解決)
	float spawnRadius = 10.0f;							// 自分を中心にこの半径の球内へばらまく(保存される)
};

template<>
struct Engine::ECS::ComponentTraits<BoidSpownerComponent>
{
	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		BoidSpownerComponent& _comp = Engine::Editor::GetValue<BoidSpownerComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.prefab);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoidSpownerComponent& _comp = Engine::Editor::GetValue<BoidSpownerComponent>(a_pData);
		a_ar.GUIDField("boidPrefabGUID", _comp.boidPrefabGUID);
		a_ar.Field("spawnRadius", _comp.spawnRadius);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoidSpownerComponent& _comp = Engine::Editor::GetValue<BoidSpownerComponent>(a_context.pData);
		auto& _services = *a_context.pWorld->RefEngineServices();

		if (Engine::Editor::AssetField(_services, "Boid Prefab", "Prefab", _comp.boidPrefabGUID))
		{
			// GUIDが変わったら作り直し。借りていたぶんは返す
			_services.pResourceManager->ReleaseHandle(_comp.prefab);
		}
		Engine::Editor::Field("SpawnRadius", _comp.spawnRadius, 0.1f, 0.0f);
		Engine::Editor::HelpText("(spawn count is decided by the controller)");
	}
};
