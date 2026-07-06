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

void Engine::D3D12::SamplerAllocator::Release()
{
	m_pHeap = nullptr;
}

Engine::Handle<SAMPLER> Engine::D3D12::SamplerAllocator::Allocate(D3D12::Device* a_pDevice, const D3D12_SAMPLER_DESC& a_desc)
{
	// RTVの位置を取得
	if (m_indexQueue.empty())
	{
		assert(0 && "SAMPLERの使用上限に達しました");
	}
	uint16_t _idx = m_indexQueue.front(); m_indexQueue.pop();
	uint16_t _gen = m_genVec[_idx];
	Engine::Handle<SAMPLER> _handle(_idx,_gen);

	// ハンドル作成
	auto _handleCPU = m_pHeap->GetCPU(static_cast<UINT>(_handle.GetIndex()));

	// ビュー作成
	a_pDevice->CreateSampler(
		&a_desc,
		_handleCPU
	);

	return _handle;
}


void Engine::D3D12::SamplerAllocator::Remove(Engine::Handle<SAMPLER> a_handle)
{
	// 世代を上げてキューに格納
	m_genVec[a_handle.GetIndex()]++;
	m_indexQueue.push(a_handle.GetIndex());
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::SamplerAllocator::GetCPU(const Engine::Handle<SAMPLER>&a_handle) const
{
	if (m_genVec[a_handle.GetIndex()] == a_handle.GetGeneration())
	{
		return m_pHeap->GetCPU(a_handle.GetIndex());
	}
	assert(0 && "SAMPLERの世代が違います");
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::D3D12::SamplerAllocator::GetGPU(const Engine::Handle<SAMPLER>& a_handle) const
{
	if (m_genVec[a_handle.GetIndex()] == a_handle.GetGeneration())
	{
		return m_pHeap->GetGPU(a_handle.GetIndex());
	}
	assert(0 && "SAMPLERの世代が違います");
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}
