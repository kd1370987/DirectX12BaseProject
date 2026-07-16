#pragma once

struct CameraTag {};

template<>
struct Engine::ECS::ComponentTraits<CameraTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};