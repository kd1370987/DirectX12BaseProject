#pragma once
//==========================================================================================
//
// NodeGraphEditor (Engine::Editor)
//
// ImNodes を使うエディターの土台。
//
// ImNodes の作法(コンテキストの発行、BeginNodeEditor / EndNodeEditor の対、
// 線の生成は End の後でないと拾えない、座標の反映はメインスレッドでしかできない)を
// ここに閉じ込め、派生は「何を描くか」だけを書けばよいようにする。
//
// グラフの中身の型はここでは一切知らない。
// レンダーグラフの Pass もステートマシンの Node も、
// 描く側が自分の型のまま扱えるように、受け口は全部 virtual にしてある。
//
// 実体はエディター側が持つ。編集対象のアセットは持たない(GUID などで引き直す)
//
//==========================================================================================
namespace Engine::Editor
{
	struct EditorContext;

	class NodeGraphEditor
	{
	public:

		NodeGraphEditor() = default;
		virtual ~NodeGraphEditor();

		// ImNodesEditorContext* を生ポインタで所有するのでコピー・ムーブ禁止。
		// 持ち主は unique_ptr 側なので、運ぶ必要もない
		NodeGraphEditor(const NodeGraphEditor&) = delete;
		NodeGraphEditor& operator=(const NodeGraphEditor&) = delete;

		//----------------------------------------------------------------------------------
		// 1フレーム分の編集UI
		//
		// 呼ぶ順番はここが決める。派生は差し込むところだけを埋める
		//----------------------------------------------------------------------------------
		void Draw(EditorContext& a_editContext);

		// ImNodes 上の今の座標をグラフ側へ書き戻す。
		// 保存の直前に通さないと、動かしたノードの位置が保存されない
		void SyncNodePositions();

		// 次の Draw で、保存されている座標を ImNodes へ流し込ませる。
		// ImNodes はコンテキストが有効なメインスレッドでしか触れないので、
		// 読み込んだその場では反映できない
		void RequestApplyNodePositions() { m_isApplyPositionsPending = true; }

	protected:

		//----------------------------------------------------------------------------------
		// 派生が埋めるところ
		//----------------------------------------------------------------------------------
		// 描き始める前の確認 : false を返すとこのフレームは何も描かない。
		// 編集対象を引き直して、無ければ抜けるのに使う
		virtual bool OnBeginDraw(EditorContext& a_editContext) { (void)a_editContext; return true; }

		// グラフの外に出すもの(ボタン・検証結果・選択中ノードの詳細など)。
		// ここまで来た時点でコンテキストは有効なので、選択状態を見てよい
		virtual void OnDrawHeader(EditorContext& a_editContext) { (void)a_editContext; }

		// BeginNodeEditor と EndNodeEditor の間。ノードと線はここで描く
		virtual void OnDrawNodes(EditorContext& a_editContext) = 0;

		// EndNodeEditor の後。
		// 線が引かれたか・Delete キーが押されたかは、ここでしか拾えない
		virtual void OnPostDraw(EditorContext& a_editContext) { (void)a_editContext; }

		// 保存されている座標を ImNodes へ流し込む
		virtual void OnApplyNodePositions() {}

		// ImNodes の今の座標をグラフへ書き戻す
		virtual void OnSyncNodePositions() {}

		// ミニマップを出すか
		virtual bool IsShowMiniMap() const { return true; }

		//----------------------------------------------------------------------------------
		// 派生から使う道具
		//----------------------------------------------------------------------------------
		// このエディターのコンテキストを現在のものにする。
		// 複数のグラフを同時に開けるので、触る前に必ず通すこと
		void SetContextCurrent();

		// 選択中のノード / 線
		static std::vector<int> GetSelectedNodeIDs();
		static std::vector<int> GetSelectedLinkIDs();

		// ちょうど1つだけ選ばれているノード : そうでなければ 0。
		// ノードIDは 1 から配られるので 0 を「無し」に使える
		static int GetSingleSelectedNodeID();

		// このフレームで Delete キーが押されたか
		static bool IsDeleteKeyPressed();

	private:

		void EnsureContext();
		void DestroyContext();

		// 作業領域を一意に定めるもの。グラフごとに独立して持つ
		ImNodesEditorContext* m_pContext = nullptr;

		// 最初の Draw では必ず流し込む。
		// エディターは開いたときに作られるので、これが「読み込み直後」にあたる
		bool m_isApplyPositionsPending = true;
	};
}
