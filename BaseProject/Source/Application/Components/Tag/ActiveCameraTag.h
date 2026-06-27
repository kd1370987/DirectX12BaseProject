
#pragma once

struct ActiveCameraTag {};

template<>
struct Engine::ECS::ComponentTraits<ActiveCameraTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(void* a_pData) {}
};