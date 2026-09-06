#pragma once
//==========================================================================================
//
// AssetInspector (Engine::Editor::Inspector)
//
// 選択中のアセットの詳細を出す。
//
// 大半のアセットは状態を持たないので自由関数のまま(ResourceDraw)。
// ノードグラフのように「エディター側が状態を持つ」ものだけ、
// ここが専用クラスの実体を抱える。
//
// 実体を持つ必要があるのでクラスにしてある。
// 自由関数のままだと ImNodes のコンテキストや選択状態の置き場所が無く、
// アセット側(ランタイム)にエディターの都合を持たせることになる
//
//==========================================================================================
#include "../../../Internal/EditorContext.h"

namespace Engine::Editor
{
	class NodeGraphEditor;
}

namespace Engine::Editor::Inspector
{
	class AssetInspector
	{
	public:

		AssetInspector();
		~AssetInspector();

		// ノードエディターの実体を抱えるのでコピー禁止
		AssetInspector(const AssetInspector&) = delete;
		AssetInspector& operator=(const AssetInspector&) = delete;

		void Draw(EditorContext& a_editContext);

	private:

		//----------------------------------------------------------------------------------
		// 選択中のアセットに合わせてノードエディターを用意する
		//
		// 抱えるのは1つだけで、選択が変わったら作り直す。
		// アセットごとに残すとコンテキストが積み上がるので、捨てる区切りが要る
		//----------------------------------------------------------------------------------
		void SyncNodeEditor(const EditorContext& a_editContext);

		// タイプごとのアセット描画
		void DrawByType(EditorContext& a_editContext);

		// 今エディターを持っているアセット : 無効なら誰も開いていない
		Engine::GUID m_openedGUID = {};

		// ノードグラフを持つアセットの編集UI。
		// 実体を持つのはここだけで、他のアセットは自由関数のまま
		std::unique_ptr<NodeGraphEditor> m_upNodeEditor = nullptr;
	};
}
