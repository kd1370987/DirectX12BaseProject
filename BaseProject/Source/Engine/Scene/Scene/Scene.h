#pragma once

namespace Engine::ECS
{
	class World;
}
namespace Engine::Scene
{
	class BaseScene
	{
	public:

		BaseScene();
		virtual ~BaseScene();

		/// <summary>
		/// 初期化
		/// </summary>
		void Enter();

		/// <summary>
		/// 解放
		/// </summary>
		void Exit();

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update(float a_dt);

		/// <summary>
		/// 描画処理
		/// </summary>
		void Draw();

		Engine::ECS::World* RefWorld() { return m_upWorld.get(); }

	protected:

		// シーンタイプの設定
		virtual void SetSceneType() = 0;

		// シーンでの初期化・解放処理
		virtual void Init() {};
		virtual void Release() {};

		// シーンごとのECS設定
		virtual void RegistryComponent();
		virtual void RegistrySystem();
		virtual void RegistryEntity();
		virtual void RegistryResource();

		// シーン特有のイベント処理
		virtual void Event() {};

	protected:

		SceneType m_sceneType;

		std::unique_ptr<Engine::ECS::World> m_upWorld = nullptr;
	};
}