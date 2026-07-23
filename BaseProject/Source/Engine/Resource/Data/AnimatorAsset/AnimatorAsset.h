#pragma once
//==========================================================================================
//
// AnimatorAsset
//
// アニメーション用ステートマシン(=アニメーター)の「設計図」リソース。
// 設計図データ・遷移ロジックは Engine::StateGraph の再利用コアに委譲し、
// このクラスは「ステートごとに再生するアニメ」という Animator 固有の意味付けを担当する。
//
// ゲームプレイ(移動可否・無敵など行動)を決めるステートマシンは別アセット
// (ActionStateMachineAsset)として分離している。
//
// ランタイムのパラメータ実体(StateMachinInstance)はエンティティごとに
// StateMachineComponent 側のプールが持つ(このアセットは共有設計図なので保持しない)。
//
//==========================================================================================
#include "Engine/Resource/StateGraph/StateGraph.h"
#include "Engine/Editor/Widget/StateGraphEditor/StateGraphEditor.h"
#include "AdditivePoseTypes.h"

namespace Engine::Resource
{
	// Animator のステートノード。
	// 共通「つなぎ情報」を継承し、固有データとして再生アニメ情報を持つ。
	struct AnimatorNode : Engine::StateGraph::StateNodeBase
	{
		Engine::GUID			animGUID;					// セーブ用(アニメのGUID)
		ResourceRef<AnimationData> playAnimData;			// 実行時に解決される再生アニメ参照
		float					speed = 0.0f;
		bool					isLoop = false;

		// 加算ポーズの効き。ステートごとに変えられる。
		// (例: ブースト移動中は0.6、しゃがみ中は0.0)
		float					additiveWeight = 1.0f;

		void Archive(Persistence::Archive& a_arch);
	};

	// ランタイムのパラメータ実体は共通の ParamSet をそのまま使う。
	// (既存コードとの互換のため名前は StateMachinInstance のまま)
	using StateMachinInstance = Engine::StateGraph::ParamSet;

	// アニメーター設計図
	class AnimatorAsset
	{
	public:
		using Graph = Engine::StateGraph::StateGraph<AnimatorNode>;

		AnimatorAsset() = default;
		~AnimatorAsset() = default;
		NON_COPYABLE_MOVABLE(AnimatorAsset);

		// ノード情報取得
		UINT GetStateHash(const std::string& a_stateName) const { return m_graph.GetStateHash(a_stateName); }
		std::string_view GetNodeName(const UINT& a_hash) const { return m_graph.GetNodeName(a_hash); }
		const AnimatorNode* GetStateNode(UINT a_stateHash) const { return m_graph.GetStateNode(a_stateHash); }

		// 加算ポーズのボーン定義(AdditivePoseLinkSystem が nodeIdx へ解決する)
		const std::vector<AdditiveBoneDef>& GetAdditiveBones() const { return m_additiveBones; }

		// 保存と読み込み
		void Save(const std::string& a_savePath);
		void Load(const std::string& a_fileDir, const std::string& a_fileName);
		void Load(const std::string& a_filePath);

		// 解放
		void Release();

		// エディターからの呼び出し用(設計図を編集する)
		void EditImGui(const Handle<AnimatorAsset>& a_handle);

		// 名前
		void SetName(const std::string& a_name) { m_name = a_name; }
		const std::string& GetName() const { return m_name; }

		// ---- 判定ロジック ----
		UINT GetDefaultStartHash() const { return m_graph.GetDefaultStartHash(); }

		// ステート遷移判定を行い、次ステートのハッシュを返す
		UINT EvaluateNextState(UINT a_currentStateHash, StateMachinInstance& a_instance) const
		{
			return m_graph.Evaluate(a_currentStateHash, a_instance);
		}

	private:
		// 参照モデル選択UI(Animator固有)
		void BindModelComb();

		// 加算ポーズのボーン定義編集UI
		void AdditiveBoneEdit();

		// 加算ポーズのボーン定義のシリアライズ(Save/Load共通)
		void ArchiveAdditiveBones(Persistence::Archive& a_arch);

		// 共通のロード処理
		void LoadInternal(const std::string& a_fileDir, const std::string& a_fileName);

	private:
		// 参照モデル(アニメ選択用)
		Engine::GUID	m_modelGUID = Engine::DefaultGUID;
		Handle<Model>	m_modelHandle = {};

		// 識別子
		std::string		m_name;

		// 設計図(再利用コア)
		Graph			m_graph;

		// 加算ポーズの対象ボーン定義(モデル依存の構造情報)
		std::vector<AdditiveBoneDef> m_additiveBones;

		// 編集UI(汎用ウィジェット)
		Engine::Editor::StateGraphEditor<AnimatorNode> m_editor;
	};
}
