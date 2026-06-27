
#pragma once

struct StartTag {};

template<>
struct Engine::ECS::ComponentTraits<StartTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(void* a_pData) {}
};