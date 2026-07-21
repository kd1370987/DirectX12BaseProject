#pragma once
//==========================================================================================
//
// StateGraph<TNode>
//
// ステートグラフの「設計図データ」と「遷移ロジック」を担う再利用コア。
// UI(ImNodes描画)は持たない。描画は Editor 側の汎用ウィジェットが担当する。
//
// TNode は StateNodeBase を継承し、void Archive(Persistence::Archive&) を持つこと。
// マシン固有データ(再生アニメ / シーンGUID / アクション設定など)は TNode 側に載せる。
//
// マシン固有のヘッダ項目(参照モデルGUID・初期シーンGUIDなど)はこのクラスには入れず、
// 各マシンが自分の Save/Load でアーカイブし、グラフ部分だけ SaveGraph/LoadGraph に委譲する。
//
//==========================================================================================
#include "StateGraphTypes.h"

namespace Engine::StateGraph
{
	template<class TNode>
	class StateGraph
	{
		static_assert(std::is_base_of_v<StateNodeBase, TNode>,
			"TNode は StateNodeBase を継承している必要があります");

	public:
		StateGraph() = default;
		~StateGraph() = default;

		//----------------------------------------------------------------------------------
		// 参照アクセス(Editorウィジェットが編集に使う)
		//----------------------------------------------------------------------------------
		std::unordered_map<UINT, TNode>& Nodes() { return m_nodeMap; }
		const std::unordered_map<UINT, TNode>& Nodes() const { return m_nodeMap; }

		std::unordered_map<UINT, std::vector<TransitionArrow>>& Arrows() { return m_arrowMap; }
		const std::unordered_map<UINT, std::vector<TransitionArrow>>& Arrows() const { return m_arrowMap; }

		std::unordered_map<UINT, StateParameter>& Parameters() { return m_parameters; }
		const std::unordered_map<UINT, StateParameter>& Parameters() const { return m_parameters; }

		//----------------------------------------------------------------------------------
		// ID / 既定ステート
		//----------------------------------------------------------------------------------
		int GenerateID() { return ++m_idCounter; }

		UINT GetDefaultStartHash() const { return m_defaultStartHash; }
		void SetDefaultStartHash(UINT a_hash) { m_defaultStartHash = a_hash; }

		//----------------------------------------------------------------------------------
		// ノード操作
		//----------------------------------------------------------------------------------
		// 名前からノードを新規作成して参照を返す(既存挙動: 最初のノードを既定開始に)
		TNode& AddNode(const std::string& a_name)
		{
			TNode _node;
			_node.name = a_name;
			_node.hash = StringUtility::ToHash(a_name);
			_node.nodeID = GenerateID();
			_node.inPinID = GenerateID();
			_node.outPinID = GenerateID();

			if (m_defaultStartHash == 0)
			{
				m_defaultStartHash = _node.hash;
			}

			auto _result = m_nodeMap.insert_or_assign(_node.hash, std::move(_node));
			return _result.first->second;
		}

		// 名前からハッシュを引く。無ければ UINT_MAX。
		// ノードを削除する。
		// このノードに出入りする遷移矢印もすべて巻き添えで削除し、
		// 既定開始ステートだった場合は残りのノードへ付け替える。
		void RemoveNode(UINT a_hash)
		{
			m_nodeMap.erase(a_hash);

			// このノードから出る矢印を削除
			m_arrowMap.erase(a_hash);

			// このノードへ入る矢印を削除
			for (auto& [_src, _arrowVec] : m_arrowMap)
			{
				_arrowVec.erase(
					std::remove_if(_arrowVec.begin(), _arrowVec.end(),
						[a_hash](const TransitionArrow& a) { return a.dstStartHash == a_hash; }),
					_arrowVec.end());
			}

			// 既定開始ステートが消えたなら残りの先頭へ付け替え(無ければ0)
			if (m_defaultStartHash == a_hash)
			{
				m_defaultStartHash = m_nodeMap.empty() ? 0 : m_nodeMap.begin()->first;
			}
		}

		// パラメータを削除する。
		// このパラメータを参照している遷移条件もすべて削除する。
		void RemoveParameter(UINT a_hash)
		{
			m_parameters.erase(a_hash);

			for (auto& [_src, _arrowVec] : m_arrowMap)
			{
				for (auto& _arrow : _arrowVec)
				{
					_arrow.conditions.erase(
						std::remove_if(_arrow.conditions.begin(), _arrow.conditions.end(),
							[a_hash](const TransitionCondition& c) { return c.paramHash == a_hash; }),
						_arrow.conditions.end());
				}
			}
		}

		UINT GetStateHash(const std::string& a_name) const
		{
			UINT _hash = StringUtility::ToHash(a_name);
			auto _it = m_nodeMap.find(_hash);
			return (_it != m_nodeMap.end()) ? _it->first : UINT_MAX;
		}

		std::string_view GetNodeName(UINT a_hash) const
		{
			auto _it = m_nodeMap.find(a_hash);
			return (_it != m_nodeMap.end()) ? std::string_view(_it->second.name) : std::string_view();
		}

		const TNode* GetStateNode(UINT a_hash) const
		{
			auto _it = m_nodeMap.find(a_hash);
			return (_it != m_nodeMap.end()) ? &_it->second : nullptr;
		}
		TNode* GetStateNode(UINT a_hash)
		{
			auto _it = m_nodeMap.find(a_hash);
			return (_it != m_nodeMap.end()) ? &_it->second : nullptr;
		}

		//----------------------------------------------------------------------------------
		// 遷移評価(共有アルゴリズムへ委譲)
		//----------------------------------------------------------------------------------
		UINT Evaluate(UINT a_currentHash, ParamSet& a_instance) const
		{
			return EvaluateTransition(m_arrowMap, m_parameters, a_currentHash, a_instance);
		}

		//----------------------------------------------------------------------------------
		// クリア
		//----------------------------------------------------------------------------------
		void Clear()
		{
			m_nodeMap.clear();
			m_arrowMap.clear();
			m_parameters.clear();
			m_defaultStartHash = 0;
			m_idCounter = 0;
		}

		//----------------------------------------------------------------------------------
		// グラフ部分の保存/復元(開いた Archive を受け取る)
		// マシン固有のヘッダ項目は呼び出し側で別途アーカイブすること。
		//----------------------------------------------------------------------------------
		void SaveGraph(Persistence::Archive& a_arch)
		{
			a_arch.Field("m_defaultStartHash", m_defaultStartHash);

			// ノード
			size_t _nodeSize = m_nodeMap.size();
			if (a_arch.BeginArray("StateNodes", _nodeSize))
			{
				size_t _idx = 0;
				for (auto& [_hash, _node] : m_nodeMap)
				{
					if (a_arch.BeginObject(_idx))
					{
						_node.Archive(a_arch);
						a_arch.EndObject();
					}
					++_idx;
				}
				a_arch.EndArray();
			}

			// 遷移矢印(ノードごとにまとめる)
			size_t _arrowMapSize = m_arrowMap.size();
			if (a_arch.BeginArray("TransitionArrows", _arrowMapSize))
			{
				size_t _idx = 0;
				for (auto& [_hash, _arrowVec] : m_arrowMap)
				{
					if (a_arch.BeginObject(_idx))
					{
						UINT _h = _hash;
						a_arch.Field("SrcHash", _h);

						size_t _arrowSize = _arrowVec.size();
						if (a_arch.BeginArray("Arrows", _arrowSize))
						{
							for (size_t _j = 0; _j < _arrowSize; ++_j)
							{
								if (a_arch.BeginObject(_j))
								{
									_arrowVec[_j].Archive(a_arch);
									a_arch.EndObject();
								}
							}
							a_arch.EndArray();
						}
						a_arch.EndObject();
					}
					++_idx;
				}
				a_arch.EndArray();
			}

			// パラメータ
			size_t _paramSize = m_parameters.size();
			if (a_arch.BeginArray("Parameters", _paramSize))
			{
				size_t _idx = 0;
				for (auto& [_hash, _param] : m_parameters)
				{
					if (a_arch.BeginObject(_idx))
					{
						UINT _h = _hash;
						a_arch.Field("Hash", _h);
						_param.Archive(a_arch);
						a_arch.EndObject();
					}
					++_idx;
				}
				a_arch.EndArray();
			}
		}

		void LoadGraph(Persistence::Archive& a_arch)
		{
			Clear();

			a_arch.Field("m_defaultStartHash", m_defaultStartHash);

			int _maxId = 0;

			// ノード復元
			size_t _nodeSize = 0;
			if (a_arch.BeginArray("StateNodes", _nodeSize))
			{
				for (size_t _i = 0; _i < _nodeSize; ++_i)
				{
					if (a_arch.BeginObject(_i))
					{
						TNode _node;
						_node.Archive(a_arch);

						UINT _key = StringUtility::ToHash(_node.name);
						_node.hash = _key;

						// ID未設定なら採番。既存IDは最大値追跡に使う。
						if (_node.nodeID == 0)	_node.nodeID = ++_maxId;	else _maxId = std::max(_maxId, _node.nodeID);
						if (_node.inPinID == 0)	_node.inPinID = ++_maxId;	else _maxId = std::max(_maxId, _node.inPinID);
						if (_node.outPinID == 0) _node.outPinID = ++_maxId;	else _maxId = std::max(_maxId, _node.outPinID);

						m_nodeMap.emplace(_key, std::move(_node));
						a_arch.EndObject();
					}
				}
				a_arch.EndArray();
			}

			// 矢印復元
			size_t _arrowMapSize = 0;
			if (a_arch.BeginArray("TransitionArrows", _arrowMapSize))
			{
				for (size_t _i = 0; _i < _arrowMapSize; ++_i)
				{
					if (a_arch.BeginObject(_i))
					{
						UINT _hash = 0;
						a_arch.Field("SrcHash", _hash);

						size_t _arrowSize = 0;
						std::vector<TransitionArrow> _arrowVec;
						if (a_arch.BeginArray("Arrows", _arrowSize))
						{
							_arrowVec.resize(_arrowSize);
							for (size_t _j = 0; _j < _arrowSize; ++_j)
							{
								if (a_arch.BeginObject(_j))
								{
									_arrowVec[_j].Archive(a_arch);
									if (_arrowVec[_j].linkID == 0)	_arrowVec[_j].linkID = ++_maxId;
									else							_maxId = std::max(_maxId, _arrowVec[_j].linkID);
									a_arch.EndObject();
								}
							}
							a_arch.EndArray();
						}
						m_arrowMap.emplace(_hash, std::move(_arrowVec));
						a_arch.EndObject();
					}
				}
				a_arch.EndArray();
			}

			// パラメータ復元
			size_t _paramSize = 0;
			if (a_arch.BeginArray("Parameters", _paramSize))
			{
				for (size_t _i = 0; _i < _paramSize; ++_i)
				{
					if (a_arch.BeginObject(_i))
					{
						UINT _hash = 0;
						a_arch.Field("Hash", _hash);

						StateParameter _param;
						_param.Archive(a_arch);
						m_parameters.emplace(_hash, _param);

						a_arch.EndObject();
					}
				}
				a_arch.EndArray();
			}

			m_idCounter = _maxId;
		}

	private:
		UINT m_defaultStartHash = 0;

		std::unordered_map<UINT, TNode>								m_nodeMap;		// hash -> ノード
		std::unordered_map<UINT, std::vector<TransitionArrow>>	m_arrowMap;		// srcHash -> 矢印群
		std::unordered_map<UINT, StateParameter>				m_parameters;	// hash -> パラメータ定義

		int m_idCounter = 0;
	};
}
