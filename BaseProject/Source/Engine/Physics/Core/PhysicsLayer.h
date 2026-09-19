#pragma once


#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Engine::Physics
{
	// Jolt側でBodyを分離するLayer
	enum class EObjectLayer : JPH::ObjectLayer
	{
		NonMoving =0,
		Moving,

		NumLayers
	};

	// BroadPhaseで分類するLayer
	namespace EBroadPhaseLayer
	{
		inline constexpr JPH::BroadPhaseLayer Static{ 0 };
		inline constexpr JPH::BroadPhaseLayer Dynamic{ 1 };

		inline constexpr JPH::uint NumLayers = 2;
	}
}