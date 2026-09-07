#pragma once
//==========================================================================================
//
// PassEditor (Engine::Editor::Inspector)
//
// パス1種類ぶんの編集UI。
//
// 以前は Pass::EditNode() / EditUpdate() という純粋仮想で、
// ランタイムのパス30個すべてが ImGui を直接呼んでいた。
// ここへ移して、パス側は「何を持っているか」だけを公開する形にしてある。
//
// 対応付けは C++ の型(type_index)で引く。
// パス型IDでもよいが、それだと PassMetaRegistry を通す必要があり、
// 編集UIを引くだけのために依存が1本増える
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
{
	class Pass;
	enum class EPassEditResult : uint8_t;
}

namespace Engine::Editor::Inspector
{
	//======================================================================================
	// 編集UIの受け口
	//
	// 実体は型ごとに1つで、状態を持たない。
	// 編集対象は毎回引数で渡される
	//======================================================================================
	class IPassEditor
	{
	public:

		virtual ~IPassEditor() = default;

		// ノードの中に出すもの。
		// ここに詰めすぎると線が見えなくなるので、小さい表示だけにすること
		virtual void DrawNode(Graphics::Pipeline::Pass& a_pass) = 0;

		// 選択中パスの詳細。戻り値は「どこまで反映し直す必要があるか」
		//   Param     : 値を写すだけでよいもの(色・強度など)
		//   Structure : フォーマットやスケールのようにリソースの要件が変わるもの
		// 返し忘れると、設計図だけ変わって画面が変わらない状態になる
		virtual Graphics::Pipeline::EPassEditResult DrawDetail(Graphics::Pipeline::Pass& a_pass) = 0;
	};

	//======================================================================================
	// 型付きの土台
	//
	// 派生は自分のパス型のまま受け取れる。
	// 引かれた時点で型は一致しているので、ここでのキャストは安全
	//======================================================================================
	template<class TPass>
	class PassEditor : public IPassEditor
	{
	public:

		void DrawNode(Graphics::Pipeline::Pass& a_pass) final
		{
			OnDrawNode(static_cast<TPass&>(a_pass));
		}

		Graphics::Pipeline::EPassEditResult DrawDetail(Graphics::Pipeline::Pass& a_pass) final
		{
			return OnDrawDetail(static_cast<TPass&>(a_pass));
		}

	protected:

		virtual void OnDrawNode(TPass& a_pass) { (void)a_pass; }
		virtual Graphics::Pipeline::EPassEditResult OnDrawDetail(TPass& a_pass) = 0;
	};

	//======================================================================================
	// パスの型 -> 編集UI
	//======================================================================================
	class PassEditorRegistry
	{
	public:

		PassEditorRegistry() = default;
		~PassEditorRegistry() = default;

		PassEditorRegistry(const PassEditorRegistry&) = delete;
		PassEditorRegistry& operator=(const PassEditorRegistry&) = delete;

		// パス型と編集UIを結びつける。
		// 引数はそのまま編集UIのコンストラクタへ渡る
		// (説明文だけが違うものを1つの実装で使い回すため)
		template<class TPass, class TEditor, class... TArgs>
		void Register(TArgs&&... a_args)
		{
			static_assert(std::is_base_of_v<PassEditor<TPass>, TEditor>,
				"TEditor は PassEditor<TPass> を継承している必要があります");

			m_editorMap.insert_or_assign(
				std::type_index(typeid(TPass)),
				std::make_unique<TEditor>(std::forward<TArgs>(a_args)...));
		}

		// 実体の型から引く : 登録が無ければ nullptr
		IPassEditor* Find(const Graphics::Pipeline::Pass& a_pass) const;

	private:

		std::unordered_map<std::type_index, std::unique_ptr<IPassEditor>> m_editorMap = {};
	};

	//--------------------------------------------------------------------------------------
	// エンジン標準のパスの編集UIをまとめて登録する。
	// これを通していないと、ノードの中身と選択中パスの詳細が空のままになる
	//--------------------------------------------------------------------------------------
	void RegisterBuiltinPassEditors(PassEditorRegistry& a_registry);
}
