
#pragma once

struct AwekeTag {};

template<>
struct Engine::ECS::ComponentTraits<AwekeTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(void* a_pData) {}
};