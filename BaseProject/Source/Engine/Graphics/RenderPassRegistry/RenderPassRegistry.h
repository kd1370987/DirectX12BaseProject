#pragma once

#include "../RenderGraph/RGData/RenderPassNode.h"

namespace Engine::Graphics
{
	/// <summary>
	/// レンダーパスを登録しておく場所
	/// </summary>
	class RenderPassRegistry
	{
	public:

		struct NodeState
		{
			UINT nameHash = 0;			// ノード名のハッシュ値
			uint64_t bitFlg = 0;		// ノードのビット番号 (64個のパスに対応するため64ビット化)
		};

		/// <summary>
		/// パスを登録して、ハッシュ値とビット番号のステートを作成する
		/// </summary>
		/// <param name="a_node">レンダーパスノード</param>
		NodeState RegisterPass(const RenderPassNode& a_node);

		// ---- アクセサ ----
		RenderPassNode* RefNode(const std::string& a_name);
		RenderPassNode* RefNode(UINT a_nameHash);
		RenderPassNode* RefNode(uint64_t a_bit);

		std::vector<std::unique_ptr<RenderPassNode>>& RefPassNodes() { return m_renderPassNodes; }

	private:

		// パスの実体
		std::vector<std::unique_ptr<RenderPassNode>> m_renderPassNodes = {};
		std::vector<NodeState>		m_nodeStates = {};

		// 辞書
		std::unordered_map<UINT, size_t> m_hashNodeMap = {};		// ハッシュ値から、パスの配列のインデックス
		std::unordered_map<uint64_t, size_t> m_bitNodeMap = {};		// ビットフラグから、パスの配列のインデックス

		// 次に登録するパスに割り当てるビット位置 (0 ～ 63)
		uint8_t m_nextBitIndex = 0;
	};
}