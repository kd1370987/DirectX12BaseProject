#pragma once

#include "../Internal/EditorContext.h"
#include "../Panel/IPanel.h"

namespace Engine::Editor
{
	class EditorCamera;
	class Profiler;

	/// <summary>
	/// パネルを管理するためのクラス
	/// </summary>
	class PanelManager
	{
	public:

		/// <summary>
		/// パネルの登録
		/// </summary>
		void Init(EditorCamera* a_pEditorCamera, Profiler* a_pProfiler);

		/// <summary>
		/// パネルの描画
		/// </summary>
		void OnDrawPanels();

		/// <summary>
		/// シーンに紐づく選択状態を捨てる(シーン切り替え時に呼ぶ)
		/// </summary>
		void ClearSceneContext();

		/// <summary>
		/// パネルの登録
		/// </summary>
		/// <typeparam name="T">型</typeparam>
		template<typename T>
		void RegisterPanel();

		/// <summary>
		/// 登録済みパネルの取得
		/// パネルは登録後に増減しないので、返したポインタは保持してよい
		/// </summary>
		/// <typeparam name="T">取得したいパネルの型</typeparam>
		/// <returns>見つからなければ nullptr</returns>
		template<typename T>
		T* RefPanel();

	private:

		/// <summary>
		/// 選択中のものが今のシーンにまだ在るかを確かめ、無ければ選択を外す。
		///
		/// パネル側にも同じ検証はあるが、あちらは ImGui::Begin が false
		/// (畳まれている・タブが裏)だと呼ばれないので当てにできない。
		/// 描画の前にここで一度だけ通す
		/// </summary>
		void ValidateContext();

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

	template<typename T>
	inline T* PanelManager::RefPanel()
	{
		for (auto& _upPanel : m_upPanelVec)
		{
			if (auto* _pTarget = dynamic_cast<T*>(_upPanel.get()))
			{
				return _pTarget;
			}
		}
		return nullptr;
	}
}