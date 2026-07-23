#pragma once

#include "../BaseObject/BaseObject.h"

namespace Engine::GameObject
{
	/// <summary>
	/// オブジェクト指向でゲーム内オブジェクトのスクリーンUIなど、
	/// ECSと相性の悪いオブジェクトを管理するクラス。
	/// シーンに持たせる
	/// </summary>
	class GameObjectManager
	{
	public:
		GameObjectManager();
		~GameObjectManager();
		NON_COPYABLE_NON_MOVABLE(GameObjectManager);

		/// <summary>
		/// オブジェクトの追加
		/// </summary>
		/// <typeparam name="T">クラス型</typeparam>
		/// <returns>追加した際のポインタ</returns>
		template<typename T>
		T* AddObject(ObjectContext& a_context);

		/// <summary>
		/// 全オブジェクトの更新
		/// </summary>
		/// <param name="a_dt">デルタタイム</param>
		void Update(ObjectContext& a_context);

		/// <summary>
		/// 全オブジェクトの描画
		/// </summary>
		/// <param name="a_dt">デルタタイム</param>
		void Draw(ObjectContext& a_context);

	private:

		std::vector<std::unique_ptr<BaseObject>> m_upObjectVec = {};
	};


	template<typename T>
	inline T* GameObjectManager::AddObject(ObjectContext& a_context)
	{
		// ベースオブジェクトの継承がされているかのチェック
		static_assert(std::is_base_of_v<BaseObject,T>);

		// オブジェクトの追加
		auto _upObject = std::make_unique<T>();
		m_upObjectVec.push_back(std::move(_upObject));
		_upObject->Init(a_context);
		return m_upObjectVec.back().get();
	}
}