#pragma once

struct MainGunTag {};

template<>
struct Engine::ECS::ComponentTraits<MainGunTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};