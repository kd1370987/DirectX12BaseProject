#pragma once
namespace Engine::ECS
{
	class World;

	struct CompEditContext
	{
		void* pData = nullptr;
		World* pWorld = nullptr;
		Entity entity = Limits::INVALID_ENTITY;
	};
}