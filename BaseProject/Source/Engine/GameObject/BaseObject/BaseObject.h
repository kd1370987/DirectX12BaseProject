#pragma once
namespace Engine::GameObject
{
	/// <summary>
	/// 引数で持たせる
	/// </summary>
	struct ObjectContext
	{
		float dt;
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
		virtual void Archive(Persistence::Archive& a_ar) {}

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
		virtual void DrawInspector() {}

		/// <summary>
		/// シーンビュー上でギズモ編集する場合にオーバーライドする。
		/// </summary>
		/// <param name="a_ctx">カメラ行列・ビューポート情報</param>
		/// <returns>ギズモを表示・操作したなら true</returns>
		virtual bool DrawGizmo(const ObjectGizmoContext& a_ctx) { return false; }

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

	protected:

		// 存在フラグ : trueにしたら次フレームの初めにオブジェクトが消去される
		bool m_isExpired = false;

		// インスタンスGUID(シーン保存時に発行し、読み込み時の参照解決に使う)
		Engine::GUID m_guid = {};
	};
}
