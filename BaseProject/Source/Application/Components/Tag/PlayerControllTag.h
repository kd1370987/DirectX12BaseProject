#pragma once

struct PlayerControllTag{};

template<>
struct Engine::ECS::ComponentTraits<PlayerControllTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};