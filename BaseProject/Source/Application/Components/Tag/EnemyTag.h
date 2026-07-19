#pragma once
struct EnemyTag {}; 

template<>
struct Engine::ECS::ComponentTraits<EnemyTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};