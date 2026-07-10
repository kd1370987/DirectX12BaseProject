#pragma once

#include "../PanelManager/EditorContext.h"

namespace Engine::Editor
{
	/// <summary>
	/// エディター上でパネルとして表示するのならこのクラスを継承させる
	/// </summary>
	class IPanel
	{
	public:
		virtual ~IPanel() = default;
		virtual const char* GetName() const = 0;						// パネル名
		virtual void OnDrawImGui(EditorContext& a_editContext) = 0;		// 実際のパネルUI描画
		bool m_isOpen = true;											// メニューバーから開閉するためのフラグ
	};
}