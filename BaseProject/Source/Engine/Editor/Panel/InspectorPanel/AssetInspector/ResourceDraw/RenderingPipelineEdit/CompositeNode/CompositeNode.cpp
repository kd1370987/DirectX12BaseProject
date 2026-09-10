#include "CompositeNode.h"

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"
#include "Engine/Graphics/RenderingPipeline/Internal/Connection.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"

namespace Engine::Editor::Inspector
{
	using namespace Engine::Graphics::Pipeline;

	//======================================================================================
	// まとまり
	//======================================================================================
	bool CompositeGroup::Contains(const Engine::GUID& a_passGUID) const
	{
		for (const Pass* _pPass : members)
		{
			if (_pPass && _pPass->GetGUID() == a_passGUID) return true;
		}
		return false;
	}

	const CompositeGroup* CompositeGroupTable::Find(const Engine::GUID& a_passGUID) const
	{
		auto _it = passToGroup.find(a_passGUID);
		if (_it == passToGroup.end()) return nullptr;
		if (_it->second >= groups.size()) return nullptr;

		return &groups[_it->second];
	}

	CompositeGroupTable BuildCompositeGroups(RenderGraph& a_graph)
	{
		CompositeGroupTable _table = {};

		// まとまりの識別子 -> groups の添字
		std::unordered_map<Engine::GUID, size_t> _guidToIndex = {};

		for (auto& _upPass : a_graph.RefPasses())
		{
			if (!_upPass) continue;
			if (!_upPass->IsInEditorGroup()) continue;

			const Engine::GUID& _groupGUID = _upPass->GetEditorGroupGUID();

			auto _it = _guidToIndex.find(_groupGUID);
			if (_it == _guidToIndex.end())
			{
				CompositeGroup _group = {};
				_group.guid = _groupGUID;
				_group.typeName = _upPass->GetEditorGroupType();

				_it = _guidToIndex.emplace(_groupGUID, _table.groups.size()).first;
				_table.groups.push_back(std::move(_group));
			}

			_table.groups[_it->second].members.push_back(_upPass.get());
			_table.passToGroup.emplace(_upPass->GetGUID(), _it->second);
		}

		// 段順に並べる。
		// パス配列の並びは追加順なので、段を増やすと順番が入れ替わることがある
		for (CompositeGroup& _group : _table.groups)
		{
			std::sort(_group.members.begin(), _group.members.end(),
				[](const Pass* a_pLhs, const Pass* a_pRhs)
				{ return a_pLhs->GetEditorGroupIndex() < a_pRhs->GetEditorGroupIndex(); });
		}

		return _table;
	}

	//======================================================================================
	// 合成ノード
	//======================================================================================
	// 既定は「先頭の入力ピンと末尾の出力ピン」。
	// 数珠つなぎのまとまりはこれで足りる
	void ICompositeNode::CollectVisibleSlots(
		const CompositeGroup& a_group,
		std::vector<Slot*>& a_outInputVec,
		std::vector<Slot*>& a_outOutputVec) const
	{
		a_outInputVec.clear();
		a_outOutputVec.clear();

		if (Pass* _pHead = a_group.GetHead())
		{
			for (Slot& _in : _pHead->RefInputSlots()) a_outInputVec.push_back(&_in);
		}
		if (Pass* _pTail = a_group.GetTail())
		{
			for (Slot& _out : _pTail->RefOutputSlots()) a_outOutputVec.push_back(&_out);
		}
	}

	ICompositeNode* CompositeNodeRegistry::Find(const std::string& a_typeName) const
	{
		auto _it = m_nodeMap.find(a_typeName);
		if (_it == m_nodeMap.end()) return nullptr;

		return _it->second.get();
	}

	//======================================================================================
	// 小道具
	//======================================================================================
	namespace CompositeUtil
	{
		Pass* FindLinkSource(
			RenderGraph& a_graph,
			const Engine::GUID& a_dstPassGUID,
			uint32_t a_dstSlotID,
			uint32_t* a_pOutSrcSlotID)
		{
			if (a_pOutSrcSlotID) *a_pOutSrcSlotID = 0;

			// 接続表は出力側が鍵なので、入力から引くには舐めるしかない。
			// パスは数十個なので毎フレーム通しても問題にならない
			for (const auto& [_srcGUID, _connectionVec] : a_graph.GetConnections())
			{
				for (const Connection& _connection : _connectionVec)
				{
					if (_connection.dstPassGUID != a_dstPassGUID) continue;
					if (_connection.dstSlotID != a_dstSlotID) continue;

					if (a_pOutSrcSlotID) *a_pOutSrcSlotID = _connection.srcSlotID;
					return a_graph.FindPass(_srcGUID);
				}
			}
			return nullptr;
		}

		void CopyInputLink(
			RenderGraph& a_graph,
			const Pass& a_srcPass,
			Pass& a_dstPass,
			const std::string& a_pinName)
		{
			const uint32_t _slotID = Pass::MakeSlotID(a_pinName);

			uint32_t _srcSlotID = 0;
			Pass* _pSource = FindLinkSource(a_graph, a_srcPass.GetGUID(), _slotID, &_srcSlotID);
			if (!_pSource) return;

			a_graph.Link(_pSource->GetGUID(), _srcSlotID, a_dstPass.GetGUID(), _slotID);
		}
	}
}
