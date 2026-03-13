#include "UAVAllocator.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool Engine::D3D12::UAVAllocator::Create(
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* a_pHeap,
	UINT a_UAVStart,
	UINT a_UAVCount
)
{
	// ヒープ参照
	m_pHeap = a_pHeap;

	// インデックス行列作成
	m_genVec.resize(a_UAVCount);
	for (UINT _idx = 0; _idx < a_UAVCount; ++_idx)
	{
		m_indexQueue.push(_idx);
		m_genVec[_idx] = 0;
	}

	// UAV開始位置
	m_UAVStartIndex = a_UAVStart;
	m_UAVRangeList.Init(a_UAVCount);

	return true;
}

std::vector<Engine::Resource::Handle<UAV>> Engine::D3D12::UAVAllocator::Allocate(std::vector<UAVViewInit> a_resourceVec)
{
	// インデックス上限チェック
	if (m_indexQueue.empty())
	{
		assert(0 && "UAVの使用上限に達しました");
	}

	// リザルト用意
	std::vector<Engine::Resource::Handle<UAV>> _result = {};
	_result.resize(a_resourceVec.size());

	// インデックス取得
	auto _range = m_UAVRangeList.Allocate(a_resourceVec.size());
	ImGuiContex::Instance().AddLog("Range : %d , %d\n", _range.startIndex, _range.rangeSize);
	for (UINT _i = 0; _i < _result.size(); ++_i)
	{
		// リソースごとのハンドル取得
		auto _handle = m_pHeap->GetCPU(m_UAVStartIndex + _range.startIndex + _i);

		// ビューの仕様書作成
		D3D12_UNORDERED_ACCESS_VIEW_DESC _UAVDesc = {};
		if (a_resourceVec[_i].pDesc)
		{
			_UAVDesc = *a_resourceVec[_i].pDesc;
		}
		else
		{
			_UAVDesc.Format = a_resourceVec[_i].pResource->GetDesc().Format;
			_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			_UAVDesc.Texture2D.MipSlice = 0;
		}

		// UAV生成
		D3D12Wrapper::Instance().GetDevice()->CreateUnorderedAccessView(
			a_resourceVec[_i].pResource,
			nullptr,
			&_UAVDesc,
			_handle
		);

		// リザルト作成
		_result[_i].idx = _range.startIndex + _i;
		_result[_i].gen = m_genVec[_result[_i].idx];

		ImGuiContex::Instance().AddLog("Allocate : %d\n", _result[_i].idx);
	}

	return _result;
}

void Engine::D3D12::UAVAllocator::Remove(Engine::Resource::Handle<UAV> a_handle)
{
	if (Check(a_handle))
	{
		m_indexQueue.push(a_handle.idx);
		m_genVec[a_handle.idx]++;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::UAVAllocator::GetCPU(const Engine::Resource::Handle<UAV>&a_handle) const
{
	if (Check(a_handle))
	{
		return m_pHeap->GetCPU(m_UAVStartIndex + static_cast<UINT>(a_handle.idx));
	}
	assert(0 && "SRVの世代が違います");
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::D3D12::UAVAllocator::GetGPU(const Engine::Resource::Handle<UAV>& a_handle) const
{
	if (Check(a_handle))
	{
		return m_pHeap->GetGPU(m_UAVStartIndex + static_cast<UINT>(a_handle.idx));
	}
	assert(0 && "SRVの世代が違います");
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}

bool Engine::D3D12::UAVAllocator::Check(const Engine::Resource::Handle<UAV>& a_handle) const
{
	if (m_genVec[a_handle.idx] == a_handle.gen)
	{
		return true;
	}
	return false;
}
