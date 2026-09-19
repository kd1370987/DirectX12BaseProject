#pragma once

// ジョルトフィジックス用の前方宣言
namespace JPH
{
	class TempAllocator;
	class TempAllocatorImpl;
	class JobSystem;
	class JobSystemThreadPool;
}

namespace Engine::Physics
{
	//======================================================================================
	// Jolt の「アプリで1つ」のもの
	//
	// アロケータ・Factory・型の登録・一時メモリ・JobSystem。
	// シーンごとの空間(PhysicsSystem とレイヤー定義)は PhysicsWorld が持つ。
	//
	// 寿命 : MainEngine が持つ。Release はすべての PhysicsWorld が消えた後に呼ぶこと
	//        (シーンのワールドは SceneManager::Release、エフェクトエディターのプレビューは
	//         MainEditor::Release で消えるので、MainEngine::Release のエディター解放より後)
	//======================================================================================
	class PhysicsEngine
	{
	public:

		PhysicsEngine();
		~PhysicsEngine();
		NON_COPYABLE_NON_MOVABLE(PhysicsEngine);

		// アプリケーションで一度だけの初期化
		// a_workerCount : Jolt のワーカースレッド数(1以上)
		void Init(int a_workerCount);

		// アプリケーション終了時の解放
		void Release();

		// PhysicsSystem::Update に渡すもの(PhysicsWorld が借りる)
		JPH::TempAllocator* RefTempAllocator();
		JPH::JobSystem* RefJobSystem();

		// 生きている PhysicsWorld の数。Release 時に残っていれば寿命の順番が崩れている
		void OnWorldCreated() { ++m_liveWorldCount; }
		void OnWorldDestroyed() { --m_liveWorldCount; }

	private:

		std::unique_ptr<JPH::TempAllocatorImpl> m_upTempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> m_upJobSystem;

		int m_liveWorldCount = 0;
		bool m_isInitialized = false;
	};
}
