#pragma once

#include "EditorContext.h"
#include "../Panel/IPanel.h"

namespace Engine::Editor
{
	/// <summary>
	/// パネルを管理するためのクラス
	/// </summary>
	class PanelManager
	{
	public:

		/// <summary>
		/// パネルの登録
		/// </summary>
		void Init();

		/// <summary>
		/// パネルの描画
		/// </summary>
		void OnDrawPanels();

		/// <summary>
		/// パネルの登録
		/// </summary>
		/// <typeparam name="T">型</typeparam>
		template<typename T>
		void RegisterPanel();

	private:

		// 描画パネル配列
		std::vector<std::unique_ptr<IPanel>> m_upPanelVec = {};

		// パネル間共通メモ帳
		EditorContext m_editContext = {};
	};

	template<typename T>
	inline void PanelManager::RegisterPanel()
	{
		m_upPanelVec.push_back(std::make_unique<T>());
	}
}