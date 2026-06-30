#include "RenderPassRegistry.h"

namespace Engine::Graphics
{
	void RenderPassRegistry::RegisterPass(const RenderPassNode& a_node)
	{
		UINT _hash = StringUtility::ToHash(a_node.name);

		// 名前の重複チェック
		if (m_hashNodeMap.contains(_hash))
		{
			ENGINE_ERRLOG(false, "すでに同じ名前のレンダーパスが登録されています。");
			return;
		}

		// パス登録数の上限チェック
		if (m_nextBitIndex >= 64)
		{
			ENGINE_ERRLOG(false, "登録可能なレンダーパスの上限（64個）を超過しました。");
			return;
		}

		// パスの実体をディープコピーしてポインタ管理に変換
		auto _uniqueNode = std::make_unique<RenderPassNode>(a_node);

		// 動的にビットフラグを生成
		uint64_t _bitFlag = (1ULL << m_nextBitIndex);

		// パス自体にも自動割り当てされたインデックスを覚えさせる
		_uniqueNode->passIndex = m_nextBitIndex;

		// ベクターの末尾に追加（インデックスを取得しておく）
		size_t _currentIdx = m_renderPassNodes.size();
		m_renderPassNodes.push_back(std::move(_uniqueNode));

		// ステートの記録
		NodeState _state;
		_state.nameHash = _hash;
		_state.bitFlg = _bitFlag;
		m_nodeStates.push_back(_state);

		// 高速検索用マップの構築
		m_hashNodeMap[_hash] = _currentIdx;
		m_bitNodeMap[_bitFlag] = _currentIdx;

		// 次のパス用のビットへ進める
		m_nextBitIndex++;
	}

	RenderPassNode* RenderPassRegistry::RefNode(const std::string& a_name)
	{
		UINT hash = StringUtility::ToHash(a_name);
		return RefNode(hash);
	}

	RenderPassNode* RenderPassRegistry::RefNode(UINT a_nameHash)
	{
		auto it = m_hashNodeMap.find(a_nameHash);
		if (it != m_hashNodeMap.end())
		{
			return m_renderPassNodes[it->second].get();
		}

		ENGINE_ERRLOG(false,"指定された名前のRenderPassNodeが見つかりません");
		return nullptr;
	}

	RenderPassNode* RenderPassRegistry::RefNode(uint64_t a_bit)
	{
		auto it = m_bitNodeMap.find(a_bit);
		if (it != m_bitNodeMap.end())
		{
			return m_renderPassNodes[it->second].get();
		}
		ENGINE_ERRLOG(false, "指定されたビットフラグのRenderPassNodeは見つかりません。");
		return nullptr;
	}
}