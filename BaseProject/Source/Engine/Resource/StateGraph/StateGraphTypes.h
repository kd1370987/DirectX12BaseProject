#pragma once
//==========================================================================================
//
// StateGraph 共通型
//
// Animator / ゲームプレイFSM / GameFlow の3つで共通して使う
// 「パラメータ」「遷移条件」「遷移矢印」「ノードのつなぎ情報」「実行時パラメータ」を定義する。
// 各マシン固有のデータ（再生アニメ・シーンGUID・アクション設定など）は
// StateNodeBase を継承した各マシン側のノード構造体が持つ。
//
//==========================================================================================

namespace Engine::StateGraph
{
	//--------------------------------------------------------------------------------------
	// パラメータ
	//--------------------------------------------------------------------------------------
	enum class EParamType { Float, Int, Bool, Trigger };

	// ステートマシン全体が持つ遷移判断用パラメータの「定義」
	struct StateParameter
	{
		std::string name = "";		// パラメータ名(Speed / IsGround など)
		UINT		hash = 0;
		EParamType	type = EParamType::Float;

		// 型ごとのデフォルト値
		float	defaultFloat = 0.0f;
		int		defaultInt = 0;
		bool	defaultBool = false;

		void Archive(Persistence::Archive& a_arch);
	};

	//--------------------------------------------------------------------------------------
	// 遷移条件・遷移矢印
	//--------------------------------------------------------------------------------------
	enum class ECompareOp { Greater, Less, Equal, NotEqual, True, False };

	// 1つの遷移矢印が持つ、1つ分の比較条件
	struct TransitionCondition
	{
		UINT		paramHash = 0;			// 比較対象パラメータのハッシュ
		ECompareOp	op = ECompareOp::Greater;

		// 比較する閾値
		float	thresholdFloat = 0.0f;
		int		thresholdInt = 0;

		void Archive(Persistence::Archive& a_arch);
	};

	// ノード間の遷移矢印
	struct TransitionArrow
	{
		int		linkID = 0;
		UINT	dstStartHash = 0;					// 遷移先ステートのハッシュ

		// 遷移条件(すべて満たしたら遷移)
		std::vector<TransitionCondition> conditions;

		// 遷移時のブレンド時間。アニメ用途で使う。
		// アニメを持たないマシン(GameFlowなど)では未使用のまま0で構わない。
		float	blendDuration = 0.0f;

		void Archive(Persistence::Archive& a_arch);

		// ImNodesへ線を登録する
		void EditArrow(int a_srcOutPinID, int a_dstInPinID) const;
	};

	//--------------------------------------------------------------------------------------
	// ノードのつなぎ情報
	// 各マシン固有ノードはこれを継承し、固有データ(アニメ/シーン等)を足す
	//--------------------------------------------------------------------------------------
	struct StateNodeBase
	{
		UINT			hash = 0;			// name のハッシュ(識別子)
		std::string		name = "";

		// エディター表示用
		DXSM::Vector2	editorPos = {};
		int				nodeID = 0;		// ノード自身のID
		int				inPinID = 0;	// 入力ピン
		int				outPinID = 0;	// 出力ピン

		// 継承先の Archive から呼ぶ。つなぎ情報だけを保存/復元する。
		void ArchiveTopology(Persistence::Archive& a_arch);
	};

	//--------------------------------------------------------------------------------------
	// 実行時パラメータ
	// 設計図(パラメータ定義)に対して、実際の実行中の値を保持する箱。
	// Animator ではエンティティごと、GameFlow ではマシン自身が1つ持つ。
	//--------------------------------------------------------------------------------------
	struct ParamSet
	{
		std::unordered_map<UINT, float>	floatParams;
		std::unordered_map<UINT, int>	intParams;
		std::unordered_map<UINT, bool>	boolParams;

		void SetFloat(UINT a_hash, float a_value) { floatParams[a_hash] = a_value; }
		void SetInt(UINT a_hash, int a_value) { intParams[a_hash] = a_value; }
		void SetBool(UINT a_hash, bool a_value) { boolParams[a_hash] = a_value; }
	};

	//--------------------------------------------------------------------------------------
	// 遷移評価(共有アルゴリズム)
	// 現在ステートから伸びる矢印を順に見て、全条件を満たす最初の矢印の遷移先を返す。
	// どれも満たさなければ a_currentHash をそのまま返す(=現状維持)。
	// 満たした矢印が使った Trigger パラメータは消費(false化)する。
	// 未設定パラメータは定義のデフォルト値で a_instance に補完される。
	//--------------------------------------------------------------------------------------
	UINT EvaluateTransition(
		const std::unordered_map<UINT, std::vector<TransitionArrow>>& a_arrowMap,
		const std::unordered_map<UINT, StateParameter>& a_parameters,
		UINT a_currentHash,
		ParamSet& a_instance);
}
