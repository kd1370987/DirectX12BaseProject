#pragma once

#include "Core/BodyID.h"

// ジョルトフィジックス用の前方宣言
namespace JPH
{
	class PhysicsSystem;
	class TempAllocatorImpl;
	class JobSystemThreadPool;

	class BroadPhaseLayerInterface;
	class ObjectVsBroadPhaseLayerFilter;
	class ObjectLayerPairFilter;
}

namespace Engine::Physics
{
	class PhysicsEngine
	{
	public:

		PhysicsEngine();
		~PhysicsEngine();
		NON_COPYABLE_NON_MOVABLE(PhysicsEngine);

		void Init();

		void Update(float a_dt);

		void Release();

		// 物理判定を追加
		BodyHandle CreateBody();
		void DestoryBody(BodyHandle a_handle);



	private:

		std::unique_ptr<JPH::PhysicsSystem> m_upPhysicsSystem;
		std::unique_ptr<JPH::TempAllocatorImpl> m_upTempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> m_upJobSystem;

		std::unique_ptr<JPH::BroadPhaseLayerInterface> m_upBroadPhaseLayerInterface;
		std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_upObjectVsBroadPhaseLayerFilter;
		std::unique_ptr<JPH::ObjectLayerPairFilter> m_upObjectLayerPairFilter;

	};
}