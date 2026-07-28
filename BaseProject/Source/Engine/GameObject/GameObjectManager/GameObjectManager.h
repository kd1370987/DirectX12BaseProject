#pragma once

#include "../BaseObject/BaseObject.h"
#include "../ObjectMetaRegistry/ObjectMetaRegistry.h"

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

		/// <summary>
		/// クラスメタマネージャーに登録済みのタイプインデックスから実体を生成して追加する。
		/// (エディターの AddObject ボタン用)。新規GUIDを発行する。
		/// </summary>
		/// <param name="a_typeID">ObjectMetaRegistry に登録されたタイプID</param>
		/// <returns>追加した実体(失敗時 nullptr)</returns>
		BaseObject* AddObjectByTypeID(ObjectTypeID a_typeID);

		/// <summary>
		/// シーン保存・読み込み。
		/// 保存時 : 各オブジェクトの [タイプインデックス / GUID / データ] を書き出す。
		/// 読み込み時 : タイプインデックスからクラスを復元し、GUIDとデータを流し込む。
		/// </summary>
		void Archive(Persistence::Archive& a_ar);

		/// <summary>
		/// GUIDからインスタンスを引く(参照解決用)。無ければ nullptr。
		/// </summary>
		BaseObject* FindByGUID(const Engine::GUID& a_guid) const;

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

		/// <summary>
		/// 管理中のオブジェクト一覧を取得(エディターのヒエラルキー表示用)。
		/// </summary>
		const std::vector<std::unique_ptr<BaseObject>>& GetObjects() const { return m_upObjectVec; }

	private:

		// GUID→実体の対応表を更新しつつ末尾に追加する共通処理
		BaseObject* Register(std::unique_ptr<BaseObject> a_upObject);

	private:

		ObjectContext m_objContext = {};

		std::vector<std::unique_ptr<BaseObject>> m_upObjectVec = {};

		// GUID からインスタンスを引くための対応表
		std::unordered_map<Engine::GUID, BaseObject*> m_guidMap = {};
	};


	template<typename T>
	inline T* GameObjectManager::AddObject()
	{
		// ベースオブジェクトの継承がされているかのチェック
		static_assert(std::is_base_of_v<BaseObject,T>);

		// オブジェクトの生成
		auto _upObject = std::make_unique<T>();

		// 新規GUIDを発行(まだ持っていなければ)
		Engine::GUID _guid = {};
		_guid.Create();
		_upObject->SetGUID(_guid);

		// 追加して初期化
		T* _pObject = static_cast<T*>(Register(std::move(_upObject)));
		_pObject->Init(m_objContext);
		return _pObject;
	}
}