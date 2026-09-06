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

		/// <summary>
		/// コンストラクタ
		/// </summary>
		/// <param name="a_pWorld">
		/// 同じシーンのECSワールド。ここで受け取ったものを ObjectContext に載せて
		/// 各オブジェクトへ配るので、オブジェクト側はシングルトンを触らずに済む。
		/// </param>
		explicit GameObjectManager(Engine::ECS::World* a_pWorld);
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

		/// <summary>
		/// そのポインタが今このマネージャーの管理下にあるか。
		///
		/// エディターは選択中のオブジェクトを生ポインタで覚えているので、
		/// 破棄やシーン切り替えの後に触ると解放済みメモリを読む。
		/// 参照する前にここでアドレスの一致だけを見て確かめる
		/// (実体を触らないので、相手が死んでいても安全に判定できる)。
		/// </summary>
		bool IsManaged(const BaseObject* a_pObject) const;

		/// <summary>
		/// 全オブジェクトの更新前処理
		/// </summary>
		/// <remarks>
		/// 消える指示が出ているものを配列から外し、まだ初期化を通していないものを
		/// 通しきってから、全員の PreUpdate を回す。
		/// カーソルの取り合い(ObjectContext::cursorClaim)はここで作り直すので、
		/// 名乗りは必ず Update より前に揃う
		/// </remarks>
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

		/// <summary>
		/// オブジェクトへ配っている実行コンテキストを取得。
		/// エディターから DrawInspector / DrawGizmo を呼ぶときに渡す。
		/// </summary>
		ObjectContext& RefObjectContext() { return m_objContext; }

	private:

		// GUID→実体の対応表を更新しつつ末尾に追加する共通処理
		BaseObject* Register(std::unique_ptr<BaseObject> a_upObject);

		/// <summary>
		/// まだ初期化を通していないものを PostDeserialize -> Awake -> Start と進める
		/// </summary>
		/// <remarks>
		/// ECS の World::BeginFrame と同じで、3つのフェーズをこの1回で通しきる。
		/// 1フェーズごとに全員を回すのが要で、こうしておくと
		/// 「相手がまだ前のフェーズを通っていない」状態で次に入ることがない。
		/// (シーン読み込みの直後・エディターでの追加直後・毎フレームの頭で呼ばれる)
		/// </remarks>
		void RunInitPhases();

		// 指定フェーズに居るものだけを1回ずつ呼んで、次のフェーズへ送る
		void RunInitPhase(EObjectInitPhase a_phase);

	private:

		ObjectContext m_objContext = {};

		// 初期化を通していないものが居るか。
		// 居ないときに毎フレーム配列を舐めないようにするための札
		bool m_isPendingInit = false;

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

		// 追加して初期化(この場で3つのフェーズを通しきる)
		T* _pObject = static_cast<T*>(Register(std::move(_upObject)));
		RunInitPhases();
		return _pObject;
	}
}