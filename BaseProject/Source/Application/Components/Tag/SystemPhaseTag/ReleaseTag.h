
#pragma once

struct ReleaseTag {};

template<>
struct Engine::ECS::ComponentTraits<ReleaseTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};