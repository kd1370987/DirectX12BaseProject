#pragma once

namespace Engine::ECS
{
	class World;
}

namespace Engine::Persistence
{
	class PersistenceManager
	{
	public:
		
		// シーン単位での保存・読み込み
		void SceneSave(ECS::World* a_pWorld, std::string a_path);
		void SceneLoad(ECS::World* a_pWorld, std::string a_path);

		void SceneArchive(ECS::World* a_pWorld, Archive& a_ar);
	};
}