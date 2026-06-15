#pragma once

namespace Engine::Resource
{
	enum class EParamType { Float, Int, Bool, Trigger };

	// ステートノードが持つ遷移比較対象
	struct StateParameter
	{
		// 遷移対象名(SpeedとかIsGroundとか)
		std::string name = "";
		UINT hash = 0;
		EParamType type;
		float defaultFloat = 0.0f;
		int defaultInt = 0;
		bool defaultBool = false;

		void Archive(Persistence::Archive& a_arch, const std::string& a_filedName);
	};

	// 各ステートノード
	struct StateNode
	{
		// 識別名
		UINT hash;
		std::string	name;

		// エディター用情報
		DXSM::Vector2 editorPos = {};

		void Archive(Persistence::Archive& a_arch,const std::string& a_filedName);
		void EditNode();
	};

	enum class ECompareOp {Greater,Less,Equal,NotEqual,True,False};
	// 遷移条件
	struct TransitionCondition
	{
		UINT paramHash;	// 比較対象のノードハッシュ
		ECompareOp op;		// 比較演算子

		// 比較する閾値
		float thresholdFloat = 0.0f;
		int thresholdInt = 0;
	};


	// 遷移データ
	struct TransitionArrow
	{
		int linkID;
		UINT dstStartHash;		// 遷移先のステートハッシュ

		// 遷移条件
		std::vector<TransitionCondition> conditions;

		void Archive(Persistence::Archive& a_arch, const std::string& a_filedName);
		void EditArrow(UINT a_srcHash);
	};

	// インスタンス時データ
	struct StateMachinInstance
	{
		std::unordered_map<UINT, float> floatParams;
		std::unordered_map<UINT, int> intParams;
		std::unordered_map<UINT, bool> boolParams;
	};

	// ステートマシン全体の設計図
	// 実際のランタイムデータや運用時の設定は各コンポーネントが対応する
	class StateMachineAsset
	{
	public:

		StateMachineAsset() = default;
		~StateMachineAsset() = default;

		UINT GetStateHash(const std::string& a_stateName) const;

		// 保存と読み込み
		void Save(const std::string& a_savePath);
		void Load(const std::string& a_fileDir,const std::string& a_fileName);

		// 解放
		void Release();

		// エディターからの呼び出し用
		// ここで設計図を作る
		void EditImGui();

		// 名前
		void SetName(const std::string& a_name) { m_name = a_name; }
		const std::string& GetName()const { return m_name; }

		// GUID
		void SetGUID(const Engine::GUID& a_guid) { m_guid = a_guid; }
		const Engine::GUID& GetGUID() const { return m_guid; }

		// ---- 判定ロジック ----
		// デフォルト値を返す
		UINT GetDefaultStartHash() const { return m_defaultStartHash; }

		// ステート遷移判定を行い、次ステートのハッシュを返す
		UINT EvaluateNextState(UINT a_currentStateHash,StateMachinInstance& a_instance) const;
		

	private:

		// ノードの新規作成
		void AddNode();

		// 遷移データ作成
		void CreateArrow();

		// ノードエディタ描画
		void DrawNodeEditor();

		// Arrowポップアップ画面
		void ArrowPopUp();

		// パラメータ編集
		void EditParameters();

	private:
		// 識別子
		std::string m_name;
		Engine::GUID m_guid;

		// 初期ステート
		UINT m_defaultStartHash = 0;

		// 名前ハッシュ値、データ
		std::unordered_map<UINT, StateNode> m_stateNodeMap = {};

		// 名前ハッシュ値、変更先
		std::unordered_map<UINT, std::vector<TransitionArrow>> m_transitionArrowMap = {};

		// ステートマシン全体が持つパラメター一覧
		std::unordered_map<UINT, StateParameter> m_parameters;

		// 編集用データ
		int m_editingLinkID = 0;

	};
}
