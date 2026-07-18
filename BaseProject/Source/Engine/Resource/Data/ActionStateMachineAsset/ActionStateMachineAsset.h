#pragma once
//==========================================================================================
//
// ActionStateMachineAsset
//
// ゲームプレイ用ステートマシンの「設計図」リソース。
// アニメを選ぶ AnimatorAsset とは別に、「そのステート中に何ができるか(移動可否・無敵など)」
// という“行動”を決める。入力 → パラメータ → 状態 → 行動、というのが本来の役割。
//
// 設計図データ・遷移ロジック・ノードエディタUIは Engine::StateGraph / StateGraphEditor の
// 再利用コアに委譲し、このクラスは ActionNode(行動データ)という固有の意味付けだけを持つ。
//
//==========================================================================================
#include "Engine/Resource/StateGraph/StateGraph.h"
#include "Engine/Editor/Widget/StateGraphEditor/StateGraphEditor.h"

namespace Engine::Resource
{
	// ゲームプレイのステートノード。
	// 共通「つなぎ情報」を継承し、固有データとして“このステート中の行動制約”を持つ。
	struct ActionNode : Engine::StateGraph::StateNodeBase
	{
		bool	canMove = true;			// このステート中に移動できるか
		bool	canRotate = true;		// 向きを変えられるか
		bool	invincible = false;		// 無敵か(被弾しないか)
		float	moveSpeedScale = 1.0f;	// 移動速度の倍率

		void Archive(Persistence::Archive& a_arch);
	};

	// ゲームプレイ用ステートマシンの実行時パラメータ実体。
	// 共通 ParamSet と区別するため専用型にしておく(プールを別に持てる)。
	struct ActionStateInstance : Engine::StateGraph::ParamSet {};

	// ゲームプレイ用ステートマシン設計図
	class ActionStateMachineAsset
	{
	public:
		using Graph = Engine::StateGraph::StateGraph<ActionNode>;

		ActionStateMachineAsset() = default;
		~ActionStateMachineAsset() = default;
		NON_COPYABLE_MOVABLE(ActionStateMachineAsset);

		// ノード情報取得
		UINT GetStateHash(const std::string& a_stateName) const { return m_graph.GetStateHash(a_stateName); }
		std::string_view GetNodeName(const UINT& a_hash) const { return m_graph.GetNodeName(a_hash); }
		const ActionNode* GetStateNode(UINT a_stateHash) const { return m_graph.GetStateNode(a_stateHash); }

		// 保存と読み込み
		void Save(const std::string& a_savePath);
		void Load(const std::string& a_fileDir, const std::string& a_fileName);
		void Load(const std::string& a_filePath);

		// 解放
		void Release();

		// エディターからの呼び出し用
		void EditImGui(const Handle<ActionStateMachineAsset>& a_handle);

		// 名前
		void SetName(const std::string& a_name) { m_name = a_name; }
		const std::string& GetName() const { return m_name; }

		// ---- 判定ロジック ----
		UINT GetDefaultStartHash() const { return m_graph.GetDefaultStartHash(); }

		// ステート遷移判定を行い、次ステートのハッシュを返す
		UINT EvaluateNextState(UINT a_currentStateHash, Engine::StateGraph::ParamSet& a_instance) const
		{
			return m_graph.Evaluate(a_currentStateHash, a_instance);
		}

	private:
		void LoadInternal(const std::string& a_fileDir, const std::string& a_fileName);

	private:
		std::string	m_name;

		// 設計図(再利用コア)
		Graph		m_graph;

		// 編集UI(汎用ウィジェット)
		Engine::Editor::StateGraphEditor<ActionNode> m_editor;
	};
}
