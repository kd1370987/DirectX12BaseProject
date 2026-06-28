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

		// 保存・復元
		void  Archive(Persistence::Archive& a_ar);


		Engine::ECS::World* RefWorld() { return m_upWorld.get(); }

	private:

		// シーンごとのECS設定
		void RegistryComponent();
		void RegistrySystem();
		void RegistryEntity();
		void RegistryResource();

	private:

		std::unique_ptr<Engine::ECS::World> m_upWorld = nullptr;
	};
}