#pragma once

namespace Engine::Editor
{
	/// <summary>
	/// レンダーグラフで使われている一時リソースを描画するクラス
	/// </summary>
	class SceneManagerEditor
	{
	public:

		SceneManagerEditor() = default;
		~SceneManagerEditor() = default;

		void Init();

		void Draw(UINT a_widht, UINT a_height);
	private:

		// シーンファイル操作系
		void LoadScenePopup();			// ロード処理ポップアップ
		void SaveScenePopup();			// セーブ処理ポップアップ

	private:

		// シーンファイル操作系
		bool m_isLoadScenePopup = false;
		bool m_openLoadPopup = false;
		bool m_openSaveAsPopup = false;
		std::string m_sceneNameInput = "";


		bool m_isSaveShortcut = false;
		bool m_canOverwrite = false;
		bool m_doOverwrite = false;
	};
}