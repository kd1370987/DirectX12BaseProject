#include "PhysicsEngine.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>

#pragma comment(lib, "Jolt.lib")

namespace
{
	//--------------------------------------------------------------------------------------
	// Jolt の出力をエンジンのログへ流す。
	// 既定のままだとどこにも出ないので、RegisterTypes のバージョン照合に
	// 引っかかったとき(ビルド設定の食い違い)も理由が見えないまま落ちる
	//--------------------------------------------------------------------------------------
	void JoltTrace(const char* a_fmt, ...)
	{
		char _buf[1024];
		va_list _args;
		va_start(_args, a_fmt);
		vsnprintf(_buf, sizeof(_buf), a_fmt, _args);
		va_end(_args);

		ENGINE_LOG("[Jolt] %s", _buf);
	}

#ifdef JPH_ENABLE_ASSERTS
	bool JoltAssertFailed(const char* a_expression, const char* a_message, const char* a_file, JPH::uint a_line)
	{
		ENGINE_ERROR("[Jolt] %s(%u): (%s) %s", a_file, a_line, a_expression, a_message ? a_message : "");

		// true でブレークポイントに止まる
		return true;
	}
#endif
}

namespace Engine::Physics
{
	PhysicsEngine::PhysicsEngine()
	{}
	PhysicsEngine::~PhysicsEngine()
	{
		Release();
	}

	void PhysicsEngine::Init(int a_workerCount)
	{
		if (m_isInitialized) return;

		// メモリアロケータ : 初めに呼び出す
		JPH::RegisterDefaultAllocator();

		// ログの口
		JPH::Trace = JoltTrace;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)

		// ファクトリ
		JPH::Factory::sInstance = new JPH::Factory();

		// Joltの各種Physicsタイプを登録。
		// ライブラリとこちらのビルド設定(JPH_* の定義)が食い違っていると、ここで止まる
		JPH::RegisterTypes();

		// フィジックス更新中に使う一時メモリ : 最初だから 10MB
		m_upTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		// Jolt用のJobSystem。PhysicsSystem::Update は JobSystem が無いと動かない。
		// 自前のジョブシステムとつなぐのは後回しにして、今は Jolt 付属のスレッドプールを使う
		const int _workerCount = (std::max)(1, a_workerCount);
		m_upJobSystem = std::make_unique<JPH::JobSystemThreadPool>(
			JPH::cMaxPhysicsJobs,
			JPH::cMaxPhysicsBarriers,
			_workerCount);

		m_isInitialized = true;
		ENGINE_LOG("[Physics] Jolt を初期化しました(ワーカー %d 本)", _workerCount);
	}

	void PhysicsEngine::Release()
	{
		if (!m_isInitialized) return;

		// ワールドが残ったまま型の登録を外すと、その後の破棄で消えた Factory を見に行く
		if (m_liveWorldCount != 0)
		{
			ENGINE_ERROR("[Physics] PhysicsWorld が %d 個残ったまま Jolt を解放しようとしています", m_liveWorldCount);
		}

		m_upJobSystem.reset();
		m_upTempAllocator.reset();

		// Jolt の登録解除
		JPH::UnregisterTypes();

		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		m_isInitialized = false;
	}

	JPH::TempAllocator* PhysicsEngine::RefTempAllocator()
	{
		return m_upTempAllocator.get();
	}

	JPH::JobSystem* PhysicsEngine::RefJobSystem()
	{
		return m_upJobSystem.get();
	}
}
