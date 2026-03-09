#include "DSVAllocator.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool Engine::D3D12::DSVAllocator::Create(
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>* a_pHeap
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

Engine::Resource::Handle<DSV> Engine::D3D12::DSVAllocator::Allocate(ID3D12Resource* a_resource, D3D12_DEPTH_STENCIL_VIEW_DESC* a_pDSVDesc)
{
	// RTVの位置を取得
	if (m_indexQueue.empty())
	{
		assert(0 && "DSVの使用上限に達しました");
	}
	Engine::Resource::Handle<DSV> _handle = {};
	_handle.idx = m_indexQueue.front(); m_indexQueue.pop();
	_handle.gen = m_genVec[_handle.idx];

	// ハンドル作成
	auto _handleCPU = m_pHeap->GetCPU(static_cast<UINT>(_handle.idx));

	// ビュー作成
	D3D12Wrapper::Instance().GetDevice()->CreateDepthStencilView(
		a_resource,
		a_pDSVDesc,
		_handleCPU
	);

	return _handle;
}

void Engine::D3D12::DSVAllocator::Remove(Engine::Resource::Handle<DSV> a_handle)
{
	// 世代を上げてキューに格納
	m_genVec[a_handle.idx]++;
	m_indexQueue.push(a_handle.idx);
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::DSVAllocator::GetCPU(const Engine::Resource::Handle<DSV>& a_handle) const
{
	if (m_genVec[a_handle.idx] == a_handle.gen)
	{
		return m_pHeap->GetCPU(a_handle.idx);
	}
	assert(0 && "RTVの世代が違います");
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}
