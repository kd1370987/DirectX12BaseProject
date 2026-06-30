#pragma once
 
namespace Algorithm::Graph
{
	/// <summary>
	/// 依存関係のあるグラフを一方向の配列に解決するソート
	/// </summary>
	/// <typeparam name="Node">単体の処理</typeparam>
	/// <param name="a_nodeVec">依存関係のあるグラフ・配列</param>
	/// <param name="a_outVec">ソート後の配列</param>
	template<
		typename Node,
		typename DependencyPredicateFnc
	>
	bool TopologicalSort(
		const std::vector<std::shared_ptr<Node>>& a_nodeVec,
		std::vector<Node*>& a_outVec,
		DependencyPredicateFnc&& a_dependesFnc
	)
	{
		a_outVec.clear();

		// ノード情報
		const UINT _nodeNum = static_cast<UINT>(a_nodeVec.size());

		// 辺と頂点を構築
		std::vector<std::vector<UINT>> _edges(_nodeNum);		// 自身が依存する頂点先を記録　多←１
		std::vector<UINT> _indegree(_nodeNum,0);				// 自分に依存する頂点数を記録　１→多

		// グラフを構築
		for (UINT _i = 0; _i < _nodeNum; ++_i)
		{
			for (UINT _j = 0; _j < _nodeNum; ++_j)
			{
				// 自分と同じであれば関係性はなし
				if (_i == _j) continue;

				// 依存があればグラフに記録
				if (a_dependesFnc(*a_nodeVec[_i],*a_nodeVec[_j]))
				{
					_edges[_j].push_back(_i);
					_indegree[_i]++;
				}
			}
		}

		// ソート用にキューに一時保存
		std::queue<UINT> _queue;
		for (UINT _i = 0; _i < _nodeNum; ++_i)
		{
			// 自分に依存するノードがなければ最前実行
			if (_indegree[_i] == 0)
			{
				_queue.push(_i);
			}
		}

		// 作ったキューをもとにソートする
		while (!_queue.empty())
		{
			UINT _queueSize = static_cast<UINT>(_queue.size());
			for (UINT _i = 0; _i < _queueSize; ++_i)
			{
				// キュー内にあるのは依存先がなくなったもののみなので
				// リザルトに入れていく
				UINT _idx = _queue.front();
				_queue.pop();
				a_outVec.push_back(a_nodeVec[_idx].get());
				
				// キュー内にあるノードが依存元となっているノードの依存先数をデクリメント
				for (UINT _v : _edges[_idx])
				{
					--_indegree[_v];

					// 依存先がなくなったらキューに追加
					if (_indegree[_v] == 0)
					{
						_queue.push(_v);
					}
				}
			}
		}

		// リザルトのノード数ともらってきたノード数に違いが出れば失敗
		if (a_nodeVec.size() != a_outVec.size())
		{
			assert(0 && "トポロジカルソート失敗");
			return false;
		}

		return true;
	}

	template<
		typename Node,
		typename DependencyPredicateFnc
	>
	bool TopologicalSort(
		std::vector<Node>& a_nodeVec,
		std::vector<Node*>& a_outVec,
		DependencyPredicateFnc&& a_dependesFnc
	)
	{
		a_outVec.clear();

		// ノード情報
		const UINT _nodeNum = static_cast<UINT>(a_nodeVec.size());

		// 辺と頂点を構築
		std::vector<std::vector<UINT>> _edges(_nodeNum);		// 自身が依存する頂点先を記録　多←１
		std::vector<UINT> _indegree(_nodeNum, 0);				// 自分に依存する頂点数を記録　１→多

		// グラフを構築
		for (UINT _i = 0; _i < _nodeNum; ++_i)
		{
			for (UINT _j = 0; _j < _nodeNum; ++_j)
			{
				// 自分と同じであれば関係性はなし
				if (_i == _j) continue;

				// 依存があればグラフに記録
				if (a_dependesFnc(a_nodeVec[_i], a_nodeVec[_j]))
				{
					_edges[_j].push_back(_i);
					_indegree[_i]++;
				}
			}
		}

		// ソート用にキューに一時保存
		std::queue<UINT> _queue;
		for (UINT _i = 0; _i < _nodeNum; ++_i)
		{
			// 自分に依存するノードがなければ最前実行
			if (_indegree[_i] == 0)
			{
				_queue.push(_i);
			}
		}

		// 作ったキューをもとにソートする
		while (!_queue.empty())
		{
			UINT _queueSize = static_cast<UINT>(_queue.size());
			for (UINT _i = 0; _i < _queueSize; ++_i)
			{
				// キュー内にあるのは依存先がなくなったもののみなので
				// リザルトに入れていく
				UINT _idx = _queue.front();
				_queue.pop();
				a_outVec.push_back(&a_nodeVec[_idx]);

				// キュー内にあるノードが依存元となっているノードの依存先数をデクリメント
				for (UINT _v : _edges[_idx])
				{
					--_indegree[_v];

					// 依存先がなくなったらキューに追加
					if (_indegree[_v] == 0)
					{
						_queue.push(_v);
					}
				}
			}
		}

		// リザルトのノード数ともらってきたノード数に違いが出れば失敗
		if (a_nodeVec.size() != a_outVec.size())
		{
			assert(0 && "トポロジカルソート失敗");
			return false;
		}

		return true;
	}

	template<
		typename Node,
		typename DependencyPredicateFnc
	>
	bool TopologicalSort(
		std::vector<Node*>& a_nodeVec,
		std::vector<Node*>& a_outVec,
		DependencyPredicateFnc&& a_dependesFnc
	)
	{
		a_outVec.clear();

		// ノード情報
		const UINT _nodeNum = static_cast<UINT>(a_nodeVec.size());

		// 辺と頂点を構築
		std::vector<std::vector<UINT>> _edges(_nodeNum);		// 自身が依存する頂点先を記録　多←１
		std::vector<UINT> _indegree(_nodeNum, 0);				// 自分に依存する頂点数を記録　１→多

		// グラフを構築
		for (UINT _i = 0; _i < _nodeNum; ++_i)
		{
			for (UINT _j = 0; _j < _nodeNum; ++_j)
			{
				// 自分と同じであれば関係性はなし
				if (_i == _j) continue;

				// 依存があればグラフに記録
				if (a_dependesFnc(a_nodeVec[_i], a_nodeVec[_j]))
				{
					_edges[_j].push_back(_i);
					_indegree[_i]++;
				}
			}
		}

		// ソート用にキューに一時保存
		std::queue<UINT> _queue;
		for (UINT _i = 0; _i < _nodeNum; ++_i)
		{
			// 自分に依存するノードがなければ最前実行
			if (_indegree[_i] == 0)
			{
				_queue.push(_i);
			}
		}

		// 作ったキューをもとにソートする
		while (!_queue.empty())
		{
			UINT _queueSize = static_cast<UINT>(_queue.size());
			for (UINT _i = 0; _i < _queueSize; ++_i)
			{
				// キュー内にあるのは依存先がなくなったもののみなので
				// リザルトに入れていく
				UINT _idx = _queue.front();
				_queue.pop();
				a_outVec.push_back(a_nodeVec[_idx]);

				// キュー内にあるノードが依存元となっているノードの依存先数をデクリメント
				for (UINT _v : _edges[_idx])
				{
					--_indegree[_v];

					// 依存先がなくなったらキューに追加
					if (_indegree[_v] == 0)
					{
						_queue.push(_v);
					}
				}
			}
		}

		// リザルトのノード数ともらってきたノード数に違いが出れば失敗
		if (a_nodeVec.size() != a_outVec.size())
		{
			assert(0 && "トポロジカルソート失敗");
			return false;
		}

		return true;
	}
}