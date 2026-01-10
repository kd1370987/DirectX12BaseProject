#pragma once

enum class SystemType
{
	Init,

	Update,

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