#include "GraphHeap.h"
namespace Engine::Graphics::Pipeline
{
	void GraphHeap::Create(D3D12::Device* a_pDevice, UINT64 a_maxHeapSize)
	{
		m_maxHeapSize = a_maxHeapSize;

		// ヒープ設定作成
		D3D12_HEAP_DESC _heapDesc = {};
		_heapDesc.SizeInBytes = a_maxHeapSize;
		_heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		_heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		_heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		_heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		_heapDesc.Flags = D3D12_HEAP_FLAG_NONE;

		// ヒープ作成
		HRESULT _hr = a_pDevice->CreateHeap(&_heapDesc, IID_PPV_ARGS(&m_cpHeap));
		ENGINE_ERRLOG(FAILED(_hr),"グラフヒープの作成に失敗");
	}
}