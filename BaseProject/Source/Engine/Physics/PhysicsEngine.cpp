#include "PhysicsEngine.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#pragma comment(lib, "Jolt.lib")

#include "Core/PhysicsLayer.h"
#include "Core/PhysicsBroadPhaseLayer/PhysicsBroadPhaseLayer.h"
#include "Core/PhysicsObjectVsBroadPhaseLayerFilter/PhysicsObjectVsBroadPhaseLayerFilter.h"
#include "Core/PysicsObjectLayerPairFilter/PysicsObjectLayerPairFilter.h"

namespace Engine::Physics
{
	PhysicsEngine::PhysicsEngine()
	{}
	PhysicsEngine::~PhysicsEngine()
	{}
	void PhysicsEngine::Init()
	{
		// メモリアロケータ : 初めに呼び出す
		JPH::RegisterDefaultAllocator();

		// ファクトリ
		JPH::Factory::sInstance = new JPH::Factory();

		// Joltの各種Physicsタイプを登録
		JPH::RegisterTypes();

		// フィジックス更新中に使う一時メモリ : 最初だから 10MB
		m_upTempAllocator = std::make_unique <JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		// Jolt用のJobSystem : スレッド数は既存のジョブシステムとつなぐときに考えるから後回し
		//m_upJobSystem = std::make_unique<JPH::JobSystemThreadPool>(
		//		JPH::cMaxPhysicsJobs,
		//		JPH::cMaxPhysicsBarriers,
		//		std::thread::hardware_concurrency() - 1
		//	);

		// Collision Layer
		m_upBroadPhaseLayerInterface = std::make_unique<PhysicsBroadPhaseLayer>();

		m_upObjectVsBroadPhaseLayerFilter = std::make_unique<PhysicsObjectVsBroadPhaseLayerFilter>();

		m_upObjectLayerPairFilter = std::make_unique<PysicsObjectLayerPairFilter>();

		// フィジックスシステム
		m_upPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();

		constexpr JPH::uint maxBodies				= 65536;
		constexpr JPH::uint numBodyMutexes			= 0;
		constexpr JPH::uint maxBodyPairs			= 65536;
		constexpr JPH::uint maxContactConstraints	= 65536;

		m_upPhysicsSystem->Init(
			maxBodies,
			numBodyMutexes,
			maxBodyPairs,
			maxContactConstraints,
			*m_upBroadPhaseLayerInterface.get(),
			*m_upObjectVsBroadPhaseLayerFilter.get(),
			*m_upObjectLayerPairFilter.get()
		);

		// Gravity
		m_upPhysicsSystem->SetGravity(
			JPH::Vec3(
				0.0f,
				-9.8f,
				0.0f
			)
		);
	}

	void PhysicsEngine::Update(float a_dt)
	{
		if (!m_upPhysicsSystem) return;

		constexpr int _collisionSteps = 1;
		m_upPhysicsSystem->Update(
			a_dt,
			_collisionSteps,
			m_upTempAllocator.get(),
			m_upJobSystem.get()
		);
	}

	void PhysicsEngine::Release()
	{
		// 順に解放
		m_upPhysicsSystem.reset();

		m_upObjectLayerPairFilter.reset();
		m_upObjectVsBroadPhaseLayerFilter.reset();
		m_upBroadPhaseLayerInterface.reset();

		m_upJobSystem.reset();
		m_upTempAllocator.reset();

		// Jolt の登録解除
		if (JPH::Factory::sInstance)
		{
			JPH::UnregisterTypes();

			delete JPH::Factory::sInstance;
			JPH::Factory::sInstance = nullptr;
		}
	}
}