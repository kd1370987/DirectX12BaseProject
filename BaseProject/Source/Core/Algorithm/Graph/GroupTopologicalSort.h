#pragma once
namespace Algorithm
{
	namespace Graph
	{
		/// <summary>
		/// 依存関係のあるグラフを「同時に実行できるノードの塊」に分けるソート
		///
		/// 同じグループへ入ったノード同士には依存がない。
		/// グループは前から順に処理され、1つ前のグループが終わるまで次は始められない。
		/// </summary>
		/// <typeparam name="Node">単体の処理</typeparam>
		/// <param name="a_nodeVec">依存関係のあるグラフ・配列</param>
		/// <param name="a_outGroupVec">グループ分けされた結果</param>
		/// <param name="a_dependesFnc">a が b に依存するなら true を返す述語</param>
		/// <returns>循環参照があれば false : そのとき結果は不完全になる</returns>
		template<typename Node, typename DependencyPredicateFnc>
		bool GroupTopologicalSort(
			std::vector<Node>& a_nodeVec,
			std::vector<std::vector<Node*>>& a_outGroupVec,
			DependencyPredicateFnc&& a_dependesFnc
		)
		{
			a_outGroupVec.clear();								// 出力クリア

			// 変数準備
			size_t _nodeCount = a_nodeVec.size();				// ノード数
			std::vector<std::vector<int>> _edges(_nodeCount);	// 自身が依存する頂点先
			std::vector<int> _indegree(_nodeCount, 0);			// 自分に依存する頂点数

			// グラフ構築
			for (size_t _i = 0; _i < _nodeCount; ++_i)
			{
				// 一つの頂点に対して、つながっている頂点を求める
				for (size_t _j = 0; _j < _nodeCount; ++_j)
				{
					if (_i == _j) continue;						// 自身は除外

					// 依存があればグラフに記録
					if (a_dependesFnc(a_nodeVec[_i], a_nodeVec[_j]))
					{
						_edges[_j].push_back(static_cast<int>(_i));
						_indegree[_i]++;
					}
				}
			}

			// キューの作成 & 初期化
			std::queue<int> _queue = {};
			for (size_t _i = 0; _i < _nodeCount; ++_i)
			{
				// 依存先が 0 なら実行可能としてキューに追加
				if (_indegree[_i] == 0)
				{
					_queue.push(static_cast<int>(_i));
				}
			}

			// ソート
			size_t _sortedCount = 0;					// 実際に並べ替えられたノード数
			while (!_queue.empty())
			{
				size_t _groupSize = _queue.size();
				a_outGroupVec.push_back({});			// グループを新規追加
				a_outGroupVec.back().reserve(_groupSize);

				for (size_t _i = 0; _i < _groupSize; ++_i)
				{
					// 依存先が 0 になったシステムを並べ替えた結果に追加。
					// 添え字はキューから取り出したノード番号を使うこと。
					// ループカウンタ(_i)は「グループ内の何番目か」でしかなく、
					// これを添え字にすると常に配列の先頭から詰めてしまう
					int _front = _queue.front();
					_queue.pop();
					a_outGroupVec.back().push_back(&a_nodeVec[_front]);
					++_sortedCount;

					// キュー内の処理が依存元になっている処理の依存数をデクリメント
					for (int _edge : _edges[_front])
					{
						_indegree[_edge]--;

						// 依存先が 0 になれば処理キューとして追加
						if (_indegree[_edge] == 0)
						{
							_queue.push(_edge);
						}
					}
				}
			}

			// 循環参照があると依存数が 0 にならないノードが残り、
			// そのぶんだけ結果から抜け落ちる(=実行されないシステムが出る)。
			// 黙って消えると原因が追えないのでここで検出する
			if (_sortedCount != _nodeCount)
			{
				assert(0 && "グループトポロジカルソート失敗 : 循環参照");
				return false;
			}

			return true;
		};
	}
}
