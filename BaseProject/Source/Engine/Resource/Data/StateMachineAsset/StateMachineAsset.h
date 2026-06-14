#pragma once

namespace Engine::Resource
{
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

	// 遷移データ
	struct TransitionArrow
	{
		int linkID;
		UINT dstStartHash;		// 遷移先のステートハッシュ

		void Archive(Persistence::Archive& a_arch, const std::string& a_filedName);
		void EditArrow(UINT a_srcHash);
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

	};
}
