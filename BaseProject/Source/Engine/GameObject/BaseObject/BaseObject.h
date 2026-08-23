#pragma once

namespace Engine::ECS
{
	class World;
	struct EngineServices;
}

namespace Engine::GameObject
{
	class GameObjectManager;
	/// <summary>
	/// 引数で持たせる
	///
	/// オブジェクト側からシングルトンを名指ししなくて済むように、
	/// シーン生成時にマネージャーへ差し込んだものをここで配る。
	/// (システム側の SystemContext と同じ考え方)
	/// </summary>
	struct ObjectContext
	{
		float dt = 0.0f;

		// 自分が属するシーンのECSワールド
		Engine::ECS::World* pWorld = nullptr;

		// アプリ寿命のサービス群(グラフィックス・リソース・オプションなど)
		Engine::ECS::EngineServices* pServices = nullptr;

		// 自分を持っているマネージャー。
		// オブジェクト同士を GUID で参照する(FindByGUID)ときに使う。
		// シングルトンを名指ししないための経路なので、必ずここから引くこと
		GameObjectManager* pObjectManager = nullptr;
	};

	/// <summary>
	/// シーンビューのギズモ編集用にエディターから渡す情報。
	/// (エディター側の型に依存させたくないので、ここでは生の行列とビューポート情報だけを持つ)
	/// </summary>
	struct ObjectGizmoContext
	{
		DirectX::XMFLOAT4X4 viewMat = {};		// カメラのビュー行列
		DirectX::XMFLOAT4X4 projMat = {};		// カメラのプロジェクション行列

		DXSM::Vector2 viewportPos = {};			// シーンビュー画像の左上(スクリーン絶対座標, px)
		DXSM::Vector2 viewportSize = {};		// シーンビュー画像の表示サイズ(px)
	};

	/// <summary>
	/// 奥部ジェクトに継承させるベース
	/// </summary>
	class BaseObject
	{
	public:

		BaseObject() = default;
		virtual ~BaseObject() = default;

		virtual void Init(ObjectContext& a_context);
		virtual void Release(ObjectContext& a_context);

		virtual void Update(ObjectContext& a_context);
		virtual void Draw(ObjectContext& a_context);

		//=======================================================================
		// シリアライズ用
		//=======================================================================

		/// <summary>
		/// 保存・読み込み共通のアーカイブ処理。
		/// 派生クラスは自身のメンバをアーカイブに流し込むようオーバーライドする。
		/// (保存/読み込みの分岐は Archive クラスが内部で吸収する)
		/// </summary>
		/// <param name="a_ar">保存・読み込み両対応のアーカイブ</param>
		/// <param name="a_context">リソース再要求などに使う実行コンテキスト</param>
		virtual void Archive(Persistence::Archive& a_ar, ObjectContext& a_context) {}

		//=======================================================================
		// エディター用
		//=======================================================================

		/// <summary>
		/// ヒエラルキー/インスペクターに表示する名前。
		/// </summary>
		virtual const char* GetEditorName() const { return "GameObject"; }

		/// <summary>
		/// インスペクターに描画する編集UI。ImGuiで自由に組む。
		/// </summary>
		/// <param name="a_context">リソース参照などに使う実行コンテキスト</param>
		virtual void DrawInspector(ObjectContext& a_context) {}

		/// <summary>
		/// シーンビュー上でギズモ編集する場合にオーバーライドする。
		/// </summary>
		/// <param name="a_ctx">カメラ行列・ビューポート情報</param>
		/// <param name="a_context">リソース参照などに使う実行コンテキスト</param>
		/// <returns>ギズモを表示・操作したなら true</returns>
		virtual bool DrawGizmo(const ObjectGizmoContext& a_ctx, ObjectContext& a_context) { return false; }

		//=======================================================================

		//=======================================================================
		// 表示の切り替え
		//=======================================================================

		/// <summary>
		/// 表示するか
		/// </summary>
		/// <remarks>
		/// 出し入れに対応していないオブジェクトでは何も起きない(既定は常に表示)。
		///
		/// ここに置いてあるのは、進行役(HomeSequence など)が
		/// 「このGUIDのものを出す/隠す」と束ねるときに、
		/// 相手が UI なのか別の進行役なのかを知らずに済ませるため。
		/// 型ごとに dynamic_cast を並べると、出し入れできる種類を増やすたびに
		/// 束ねる側を直すことになる。
		/// </remarks>
		virtual bool IsVisible() const { return true; }
		virtual void SetVisible(bool a_isVisible) {}

		//=======================================================================

		/// <summary>
		/// 次フレームの初めにこのオブジェクトを破棄するよう要求する。
		/// </summary>
		void RequestDestroy() { m_isExpired = true; }

		bool IsExpired() const { return m_isExpired; }

		//=======================================================================
		// GUID : インスタンスの一意識別子(シーン内でこのGUIDから実体を引ける)
		//=======================================================================
		void SetGUID(const Engine::GUID& a_guid) { m_guid = a_guid; }
		const Engine::GUID& GetGUID() const { return m_guid; }

		//=======================================================================
		// ヒエラルキー上の親 : エディターで並びをまとめるためだけのもの
		//=======================================================================

		/// <summary>
		/// ヒエラルキーでの親
		/// </summary>
		/// <remarks>
		/// 効くのは**エディターの並びだけ**。座標も回転も表示状態も親から伝わらない。
		/// 「この確認ボックスは MissionSelect のもの」といったまとまりを付けて、
		/// オブジェクトが増えたときに一覧から探せるようにするために持たせてある。
		///
		/// 親が見つからないもの(消された・まだ読み込んでいない)は根として扱われるので、
		/// 参照が切れても一覧から消えることはない。
		///
		/// 実際の親子(座標が伝わるもの)が要るなら ECS の HierarchyComponent を使うこと。
		/// </remarks>
		const Engine::GUID& GetParentGUID() const { return m_parentGUID; }
		void SetParentGUID(const Engine::GUID& a_guid) { m_parentGUID = a_guid; }

	protected:

		// 存在フラグ : trueにしたら次フレームの初めにオブジェクトが消去される
		bool m_isExpired = false;

		// インスタンスGUID(シーン保存時に発行し、読み込み時の参照解決に使う)
		Engine::GUID m_guid = {};

		// ヒエラルキー上の親(エディターの並びだけに効く)。無効なら根
		Engine::GUID m_parentGUID = {};
	};
}
