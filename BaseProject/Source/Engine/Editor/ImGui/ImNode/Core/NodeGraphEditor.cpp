#include "NodeGraphEditor.h"

namespace Engine::Editor
{
	NodeGraphEditor::~NodeGraphEditor()
	{
		DestroyContext();
	}

	void NodeGraphEditor::Draw(EditorContext& a_editContext)
	{
		// 描く相手が居なければ、コンテキストも作らずに抜ける
		if (!OnBeginDraw(a_editContext)) return;

		// インスタンスごとにポップアップ / ウィジェットIDを分離する。
		// 同じ名前のポップアップを持つエディターを2つ開いても混ざらない
		ImGui::PushID(this);

		SetContextCurrent();

		//----------------------------------------------------------------------------------
		// 保存されている座標の反映
		//
		// ImNodes はコンテキストが有効なメインスレッドでしか触れないので、
		// 読み込んだその場ではなく、最初の Draw まで持ち越して流し込む
		//----------------------------------------------------------------------------------
		if (m_isApplyPositionsPending)
		{
			OnApplyNodePositions();
			m_isApplyPositionsPending = false;
		}

		// グラフの外に出すもの。選択状態を見るのでコンテキストを立てた後
		OnDrawHeader(a_editContext);

		ImNodes::BeginNodeEditor();
		OnDrawNodes(a_editContext);
		if (IsShowMiniMap()) ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
		ImNodes::EndNodeEditor();

		// 線が引かれたか・Delete キーが押されたかは End の後でないと拾えない
		OnPostDraw(a_editContext);

		ImGui::PopID();
	}

	void NodeGraphEditor::SyncNodePositions()
	{
		SetContextCurrent();
		OnSyncNodePositions();
	}

	void NodeGraphEditor::SetContextCurrent()
	{
		EnsureContext();
		ImNodes::EditorContextSet(m_pContext);
	}

	std::vector<int> NodeGraphEditor::GetSelectedNodeIDs()
	{
		const int _count = ImNodes::NumSelectedNodes();
		if (_count <= 0) return {};

		std::vector<int> _ids(static_cast<size_t>(_count));
		ImNodes::GetSelectedNodes(_ids.data());
		return _ids;
	}

	std::vector<int> NodeGraphEditor::GetSelectedLinkIDs()
	{
		const int _count = ImNodes::NumSelectedLinks();
		if (_count <= 0) return {};

		std::vector<int> _ids(static_cast<size_t>(_count));
		ImNodes::GetSelectedLinks(_ids.data());
		return _ids;
	}

	int NodeGraphEditor::GetSingleSelectedNodeID()
	{
		if (ImNodes::NumSelectedNodes() != 1) return 0;

		int _nodeID = 0;
		ImNodes::GetSelectedNodes(&_nodeID);
		return _nodeID;
	}

	bool NodeGraphEditor::IsDeleteKeyPressed()
	{
		return ImGui::IsKeyPressed(ImGuiKey_Delete, false);
	}

	void NodeGraphEditor::EnsureContext()
	{
		if (!m_pContext)
		{
			m_pContext = ImNodes::EditorContextCreate();
		}
	}

	void NodeGraphEditor::DestroyContext()
	{
		if (m_pContext)
		{
			ImNodes::EditorContextFree(m_pContext);
			m_pContext = nullptr;
		}
	}
}
