#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystem.h>

#include "PhysicsEngine.h"
#include "Internal/JoltMath.h"
#include "Core/PhysicsLayer.h"
#include "Core/PhysicsBroadPhaseLayer/PhysicsBroadPhaseLayer.h"
#include "Core/PhysicsObjectVsBroadPhaseLayerFilter/PhysicsObjectVsBroadPhaseLayerFilter.h"
#include "Core/PysicsObjectLayerPairFilter/PysicsObjectLayerPairFilter.h"

namespace Engine::Physics
{
	PhysicsWorld::PhysicsWorld(PhysicsEngine* a_pEngine, const PhysicsWorldDesc& a_desc)
		: m_pEngine(a_pEngine)
	{
		if (!m_pEngine)
		{
			ENGINE_ERROR("[Physics] PhysicsEngine が無いので PhysicsWorld を作れません");
			return;
		}

		// レイヤー定義
		m_upBroadPhaseLayerInterface = std::make_unique<PhysicsBroadPhaseLayer>();
		m_upObjectVsBroadPhaseLayerFilter = std::make_unique<PhysicsObjectVsBroadPhaseLayerFilter>();
		m_upObjectLayerPairFilter = std::make_unique<PysicsObjectLayerPairFilter>();

		// フィジックスシステム
		m_upPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();

		// ボディの排他はしない(ECS はシングルスレッドで、ボディを触るのはメインだけ)。
		// 0 を渡すと Jolt が既定の数を選ぶ
		constexpr JPH::uint _numBodyMutexes = 0;

		m_upPhysicsSystem->Init(
			a_desc.maxBodies,
			_numBodyMutexes,
			a_desc.maxBodyPairs,
			a_desc.maxContactConstraints,
			*m_upBroadPhaseLayerInterface,
			*m_upObjectVsBroadPhaseLayerFilter,
			*m_upObjectLayerPairFilter);

		m_upPhysicsSystem->SetGravity(Internal::ToJolt(a_desc.gravity));

		m_pEngine->OnWorldCreated();
	}

	PhysicsWorld::~PhysicsWorld()
	{
		if (!m_upPhysicsSystem) return;

		// PhysicsSystem を先に壊す(レイヤー定義を参照で持っているため)
		m_upPhysicsSystem.reset();
		m_upObjectLayerPairFilter.reset();
		m_upObjectVsBroadPhaseLayerFilter.reset();
		m_upBroadPhaseLayerInterface.reset();

		m_pEngine->OnWorldDestroyed();
	}

	void PhysicsWorld::Update(float a_dt)
	{
		if (!m_upPhysicsSystem) return;

		// ボディが1つも無ければ何もしない(プレビューや、まだ登録が無いシーン)
		if (m_upPhysicsSystem->GetNumBodies() == 0) return;

		constexpr int _collisionSteps = 1;
		const JPH::EPhysicsUpdateError _error = m_upPhysicsSystem->Update(
			a_dt,
			_collisionSteps,
			m_pEngine->RefTempAllocator(),
			m_pEngine->RefJobSystem());

		if (_error != JPH::EPhysicsUpdateError::None)
		{
			ENGINE_WARNING("[Physics] PhysicsSystem::Update で上限を超えました(0x%x)", static_cast<uint32_t>(_error));
		}
	}

	uint32_t PhysicsWorld::GetBodyCount() const
	{
		return m_upPhysicsSystem ? m_upPhysicsSystem->GetNumBodies() : 0u;
	}
}
