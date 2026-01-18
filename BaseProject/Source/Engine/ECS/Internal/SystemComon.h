#pragma once

enum class SystemType
{
	Init,

	Input,
	PreUpdate,
	Update,
	Physics,
	Camera,
	PostUpdate,

	PreDraw,
	Draw,
};

template<typename... Systems>
struct TypeList {};

template<typename System>
struct SystemTraits
{
	using Reads = TypeList<>;
	using Writes = TypeList<>;
};