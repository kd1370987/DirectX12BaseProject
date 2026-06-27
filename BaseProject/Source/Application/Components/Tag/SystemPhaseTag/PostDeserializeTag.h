
#pragma once

struct PostDeserializeTag {};

template<>
struct Engine::ECS::ComponentTraits<PostDeserializeTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(void* a_pData) {}
};