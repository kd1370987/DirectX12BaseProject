#pragma once

#include "GameFlowStateStruct/GameFlowStateStruct.h"
#include "Engine/Resource/StateGraph/StateGraph.h"
#include "Engine/Editor/Widget/StateGraphEditor/StateGraphEditor.h"

namespace App::Game
{
	// =========================================================
	// ゲームフロー管理マシン
	//
	// 設計図データ・遷移ロジックは Engine::StateGraph の再利用コアに委譲し、
	// このクラスは「シーン遷移」というGameFlow固有の意味付けだけを担当する。
	// =========================================================
	class GameFlowStateMachine
	{
	public:
		using Graph = Engine::StateGraph::StateGraph<FlowNode>;

		GameFlowStateMachine() = default;
		~GameFlowStateMachine() = default;

		// 保存と読み込み
		void Save(const std::string& a_savePath);
		void Load(const std::string& a_filePath);
		void Release();

		// エディターからの呼び出し用
		void EditImGui();

		// ノード情報取得
		UINT GetStateHash(const std::string& a_stateName) const { return m_graph.GetStateHash(a_stateName); }
		std::string_view GetNodeName(const UINT& a_hash) const { return m_graph.GetNodeName(a_hash); }
		const FlowNode* GetStateNode(UINT a_stateHash) const { return m_graph.GetStateNode(a_stateHash); }

		// ---------------------------------------------------------
		// ランタイム用 API
		// ---------------------------------------------------------

		/// <summary>ゲーム開始時に呼ぶ。初期化も行う</summary>
		/// <returns>初めのシーンのGUID</returns>
		Engine::GUID Start();

		// パラメータ操作
		void SetTrigger(const std::string& a_triggerName);
		void SetBool(const std::string& a_name, bool a_value);
		void SetInt(const std::string& a_name, int a_value);
		void SetFloat(const std::string& a_name, float a_value);

		/// <summary>現在の状態とパラメーターから遷移を評価する</summary>
		/// <param name="out_nextSceneGUID">遷移した場合GUIDが更新される</param>
		/// <returns>遷移した場合 true</returns>
		bool Evaluate(Engine::GUID& out_nextSceneGUID);

		// 現在のステートハッシュを取得
		UINT GetCurrentStateHash() const { return m_currentStateHash; }

	private:
		// エディタ用内部関数
		void FarstSceneSetting();	// 初期シーン設定(GameFlow固有UI)

	private:
		// ---- 識別用データ ----
		std::string m_filePath = "";

		// --- 設計図(再利用コア) ---
		Graph m_graph;

		// --- 編集UI(汎用ウィジェット) ---
		Engine::Editor::StateGraphEditor<FlowNode> m_editor;

		// --- GameFlow固有の設定 ---
		Engine::GUID m_farstSceneGUID = {};
		bool m_isFarstScenePopup = false;

		// --- ランタイムデータ(実行状態) ---
		UINT m_currentStateHash = 0;
		Engine::StateGraph::ParamSet m_params;
	};
}
