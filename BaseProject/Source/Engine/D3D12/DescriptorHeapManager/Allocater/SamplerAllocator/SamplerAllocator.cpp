#include "SamplerAllocator.h"

bool Engine::D3D12::SamplerAllocator::Create(Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>* a_pHeap)
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

Engine::Resource::Handle<SAMPLER> Engine::D3D12::SamplerAllocator::Allocate(ID3D12Device* a_pDevice, const D3D12_SAMPLER_DESC& a_desc)
{
	// RTVの位置を取得
	if (m_indexQueue.empty())
	{
		assert(0 && "SAMPLERの使用上限に達しました");
	}
	Engine::Resource::Handle<SAMPLER> _handle = {};
	_handle.idx = m_indexQueue.front(); m_indexQueue.pop();
	_handle.gen = m_genVec[_handle.idx];

	// ハンドル作成
	auto _handleCPU = m_pHeap->GetCPU(static_cast<UINT>(_handle.idx));

	// ビュー作成
	a_pDevice->CreateSampler(
		&a_desc,
		_handleCPU
	);

	return _handle;
}


void Engine::D3D12::SamplerAllocator::Remove(Engine::Resource::Handle<SAMPLER> a_handle)
{
	// 世代を上げてキューに格納
	m_genVec[a_handle.idx]++;
	m_indexQueue.push(a_handle.idx);
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::SamplerAllocator::GetCPU(const Engine::Resource::Handle<SAMPLER>&a_handle) const
{
	if (m_genVec[a_handle.idx] == a_handle.gen)
	{
		return m_pHeap->GetCPU(a_handle.idx);
	}
	assert(0 && "SAMPLERの世代が違います");
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::D3D12::SamplerAllocator::GetGPU(const Engine::Resource::Handle<SAMPLER>& a_handle) const
{
	if (m_genVec[a_handle.idx] == a_handle.gen)
	{
		return m_pHeap->GetGPU(a_handle.idx);
	}
	assert(0 && "SAMPLERの世代が違います");
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}
