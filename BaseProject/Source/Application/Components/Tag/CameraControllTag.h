#pragma once

struct CameraControllTag{};

template<>
struct Engine::ECS::ComponentTraits<CameraControllTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(CompEditContext& a_context) {}
};