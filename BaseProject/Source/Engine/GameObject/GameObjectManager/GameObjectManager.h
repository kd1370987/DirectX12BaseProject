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
		T* AddObject();

		void PreUpdate();

		/// <summary>
		/// 全オブジェクトの更新
		/// </summary>
		/// <param name="a_dt">デルタタイム</param>
		void Update(float a_dt);

		/// <summary>
		/// 全オブジェクトの描画
		/// </summary>
		/// <param name="a_dt">デルタタイム</param>
		void Draw(float a_dt);

	private:

		ObjectContext m_objContext = {};

		std::vector<std::unique_ptr<BaseObject>> m_upObjectVec = {};
	};


	template<typename T>
	inline T* GameObjectManager::AddObject()
	{
		// ベースオブジェクトの継承がされているかのチェック
		static_assert(std::is_base_of_v<BaseObject,T>);

		// オブジェクトの追加
		auto _upObject = std::make_unique<T>();
		m_upObjectVec.push_back(std::move(_upObject));

		// push_back で _upObject は空になっているため、
		// 格納後の実体を取り出して初期化する
		T* _pObject = static_cast<T*>(m_upObjectVec.back().get());
		_pObject->Init(m_objContext);
		return _pObject;
	}
}