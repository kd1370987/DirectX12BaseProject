#pragma once

enum class SystemType
{
	Init,

	Input,
	PreUpdate,
	Update,
	Physics,
	PostUpdate,
	Camera,

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