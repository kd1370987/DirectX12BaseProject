
#pragma once

struct RayTag {};

template<>
struct Engine::ECS::ComponentTraits<RayTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData){}
	static void Edit(CompEditContext& a_context){}
};