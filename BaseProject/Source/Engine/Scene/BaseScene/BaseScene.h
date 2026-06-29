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

		/// <summary>
		/// アーカイブ処理
		/// </summary>
		/// <param name="a_ar">保存・読み込み両方可</param>
		void  Archive(Persistence::Archive& a_ar);

		/// <summary>
		/// 現在のワールドを取得
		/// </summary>
		/// <returns></returns>
		Engine::ECS::World* RefWorld() { return m_upWorld.get(); }


	private:

		std::unique_ptr<Engine::ECS::World> m_upWorld = nullptr;
	};
}