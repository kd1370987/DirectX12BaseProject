#include "SRVAllocator.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool Engine::D3D12::SRVAllocator::Create(
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* a_pHeap,
	UINT a_srvStart,
	UINT a_srvCount
)
{
	// ヒープ参照
	m_pHeap = a_pHeap;

	// インデックス行列作成
	m_entoryVec.resize(a_srvCount);
	for (UINT _idx = 0; _idx < a_srvCount; ++_idx)
	{
		m_indexQueue.push(_idx);
		m_entoryVec[_idx].count = 0;
		m_entoryVec[_idx].gen = 0;
	}

	// SRV開始位置
	m_srvStartIndex = a_srvStart;
	m_srvRangeList.Init(a_srvCount);

	return true;
}

Engine::Resource::HandleRange<SRV> Engine::D3D12::SRVAllocator::Allocate(
	std::vector<SRVViewInit> a_resourceVec
)
{
	// インデックス上限チェック
	if (m_indexQueue.empty())
	{
		assert(0 && "SRVの使用上限に達しました");
	}

	// ハンドルレンジ作成
	Engine::Resource::HandleRange<SRV> _handleRange = {};
	_handleRange.idx = m_indexQueue.front(); m_indexQueue.pop();
	_handleRange.gen = m_entoryVec[_handleRange.idx].gen;

	// 領域確保
	Storage::Range _range = m_srvRangeList.Allocate(static_cast<UINT>(a_resourceVec.size()));
	m_entoryVec[_handleRange.idx].start = _range.startIndex;
	m_entoryVec[_handleRange.idx].count = _range.rangeSize;

	// すべてのリソースに対してビューを作成していく
	for (UINT _i = 0; _i < _range.rangeSize; ++_i)
	{
		// リソースごとのハンドル取得
		auto _handle = m_pHeap->GetCPU(m_srvStartIndex + _range.startIndex + _i);

		// ビューの仕様書作成
		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		if (a_resourceVec[_i].pDesc)
		{
			_srvDesc = *a_resourceVec[_i].pDesc;
		}
		else
		{
			_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			_srvDesc.Texture2D.MipLevels = a_resourceVec[_i].pResource->GetDesc().MipLevels;
		}

		// SRV生成
		D3D12Wrapper::Instance().GetDevice()->CreateShaderResourceView(
			a_resourceVec[_i].pResource,
			&_srvDesc,
			_handle
		);
	}
	
	return _handleRange;
}

void Engine::D3D12::SRVAllocator::Remove(Engine::Resource::Handle<SRV> a_handle)
{
	if (m_entoryVec[a_handle.idx].gen != a_handle.gen)
	{
		return;
	}
	m_entoryVec[a_handle.idx].gen++;
	m_indexQueue.push(a_handle.idx);
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::SRVAllocator::GetCPU(
	const Engine::Resource::HandleRange<SRV>& a_handle
) const
{
	if (m_entoryVec[a_handle.idx].gen == a_handle.gen)
	{
		auto _idx = m_entoryVec[a_handle.idx].start;
		return m_pHeap->GetCPU(m_srvStartIndex + static_cast<UINT>(_idx));
	}
	assert(0 && "SRVの世代が違います");
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::D3D12::SRVAllocator::GetGPU(const Engine::Resource::HandleRange<SRV>& a_handle) const
{
	if (m_entoryVec[a_handle.idx].gen == a_handle.gen)
	{
		auto _idx = m_entoryVec[a_handle.idx].start;
		return m_pHeap->GetGPU(m_srvStartIndex + static_cast<UINT>(_idx));
	}
	assert(0 && "SRVの世代が違います");
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}
