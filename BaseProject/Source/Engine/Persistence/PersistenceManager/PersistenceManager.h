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
		void SeceneSerialize(ECS::World* a_pWorld,std::string a_path);
		void SeceneDeserialize(ECS::World* a_pWorld,std::string a_path);

	private:

	};
}