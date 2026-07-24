#include "RTVAllocator.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool Engine::D3D12::RTVAllocator::Create(
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>* a_pHeap
)
{
	// ヒープ参照
	m_pHeap = a_pHeap;

	// インデックス行列作成
	m_genVec.resize(a_pHeap->GetMaxSize());
	for (UINT _idx = 0; _idx < a_pHeap->GetMaxSize(); ++_idx)
	{
		m_indexQueue.push(_idx);
		m_genVec[_idx] = 0;
	}
	
	return true;
}

Engine::Resource::Handle<RTV> Engine::D3D12::RTVAllocator::Allocate(
	ID3D12Resource* a_resource,
	D3D12_RENDER_TARGET_VIEW_DESC* a_pRTVDesc
)
{
	// RTVの位置を取得
	if (m_indexQueue.empty())
	{
		assert(0 && "RTVの使用上限に達しました");
	}
	Engine::Resource::Handle<RTV> _handle = {};
	_handle.idx = m_indexQueue.front(); m_indexQueue.pop();
	_handle.gen = m_genVec[_handle.idx];

	// ハンドル作成
	auto _handleCPU = m_pHeap->GetCPU(static_cast<UINT>(_handle.idx));

	// ビュー作成
	D3D12Wrapper::Instance().GetDevice()->CreateRenderTargetView(
		a_resource,
		a_pRTVDesc,
		_handleCPU
	);

	return _handle;
}

void Engine::D3D12::RTVAllocator::Remove(
	Engine::Resource::Handle<RTV> a_handle
)
{
	if (m_genVec[a_handle.idx] != a_handle.gen)
	{
		return;
	}
	// 世代を上げてキューに格納
	m_genVec[a_handle.idx]++;
	m_indexQueue.push(a_handle.idx);
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::RTVAllocator::GetCPU(
	const Engine::Resource::Handle<RTV>& a_handle) const
{
	if (m_genVec[a_handle.idx] == a_handle.gen)
	{
		return m_pHeap->GetCPU(a_handle.idx);
	}
	assert(0 && "RTVの世代が違います");
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}
