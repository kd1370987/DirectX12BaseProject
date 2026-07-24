
#pragma once

struct AwakeTag {};

template<>
struct Engine::ECS::ComponentTraits<AwakeTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};