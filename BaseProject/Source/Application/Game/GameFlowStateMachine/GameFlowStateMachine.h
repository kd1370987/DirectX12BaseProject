#pragma once

#include "GameFlowStateStruct/GameFlowStateStruct.h"

namespace App::Game
{
	// =========================================================
	// ゲームフロー管理マシン
	// =========================================================
	class GameFlowStateMachine
	{
	public:
		GameFlowStateMachine() = default;
		~GameFlowStateMachine() = default;

		// 保存と読み込み
		void Save(const std::string& a_savePath);
		void Load(const std::string& a_filePath);
		void Release();

		// エディターからの呼び出し用
		void EditImGui();

		// ノード情報取得
		UINT GetStateHash(const std::string& a_stateName) const;
		std::string_view GetNodeName(const UINT& a_hash) const;
		const FlowStateNode* GetStateNode(UINT a_stateHash) const;


		// ---------------------------------------------------------
		// ランタイム用 API
		// ---------------------------------------------------------

		/// <summary>
		/// ゲームの開始時に呼ぶ。初期化も行う
		/// </summary>
		/// <returns>初めのシーンのGUIDが返る</returns>
		Engine::GUID Start();

		// パラメータ操作
		void SetTrigger(const std::string& a_triggerName);
		void SetBool(const std::string& a_name, bool a_value);
		void SetInt(const std::string& a_name, int a_value);
		void SetFloat(const std::string& a_name, float a_value);

		/// <summary>
		/// 現在の状態とパラメーターから遷移を評価する
		/// </summary>
		/// <param name="out_nextSceneGUID">遷移した場合GUIDが更新される</param>
		/// <returns>遷移した場合 true</returns>
		bool Evaluate(Engine::GUID& out_nextSceneGUID);

		// 現在のステートハッシュを取得
		UINT GetCurrentStateHash() const { return m_currentStateHash; }

		// ---------------------------------------------------------

		int GenerateID() { return ++m_idCounter; }

	private:

		// エディタ用内部関数
		void FarstSceneSetting();					// 初期シーン設定
		void AddNode();								// ノード追加
		void RessetButton();						// クリア用ボタン
		void CreateArrow();							// 矢印作成
		void DrawNodeEditor();						// ノード描画
		void ArrowPopUp();							// 矢印のポップアップウィンドウ
		void EditParameters();						// パラメーター編集
		void EditNode(FlowStateNode& a_node);		// ノード編集

	private:

		// ---- 識別用データ ----
		std::string m_filePath = "";

		// --- アセットデータ（設計図） ---
		Engine::GUID m_farstSceneGUID = {};
		UINT m_defaultStartHash = 0;
		std::unordered_map<UINT, FlowStateNode> m_stateNodeMap = {};
		std::unordered_map<UINT, std::vector<FlowTransitionArrow>> m_transitionArrowMap = {};
		std::unordered_map<UINT, FlowStateParameter> m_parameters = {};

		// 編集用データ
		int m_editingLinkID = 0;
		int m_idCounter = 0;
		bool m_isFarstScenePopup = false;

		// --- ランタイムデータ（実行状態） ---
		UINT m_currentStateHash = 0;
		std::unordered_map<UINT, float> m_currentFloatParams;
		std::unordered_map<UINT, int> m_currentIntParams;
		std::unordered_map<UINT, bool> m_currentBoolParams;

		ImNodesEditorContext* m_context; // 専用のコンテキスト
	};
}
