
#pragma once

struct ActiveTag {};

template<>
struct Engine::ECS::ComponentTraits<ActiveTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};