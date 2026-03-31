#include "GPUBuffer.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool Engine::D3D12Buffer::GPUBuffer::Create(
	D3D12_HEAP_TYPE a_heapType, 
	size_t a_bufferSize, 
	D3D12_RESOURCE_STATES a_initState,
	const void* a_pInitData
)
{
	// 初期化情報
	auto _prop = CD3DX12_HEAP_PROPERTIES(a_heapType);
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(a_bufferSize);

	// リソースの生成
	HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		a_initState,
		nullptr,
		IID_PPV_ARGS(m_cpBuffer.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		return false;
	}

	// 初期化データがある場合はマッピングしてコピー
	if (a_pInitData != nullptr)
	{
		m_cpBuffer->Map(0, nullptr, (void**)m_mappedData);
		memcpy(m_mappedData, a_pInitData, m_bufferSize);

		// アップロードヒープ以外はマッピング解除(CPUとGPUの紐づけを解除)
		if (a_heapType != D3D12_HEAP_TYPE_UPLOAD)
		{
			m_cpBuffer->Unmap(0, nullptr);
		}
	}

	// 記録
	m_bufferSize = a_bufferSize;
	m_currentState = a_initState;

	return true;
}


void Engine::D3D12Buffer::GPUBuffer::Update(const void* a_pData)
{
	memcpy(m_mappedData, a_pData, m_bufferSize);
}

void Engine::D3D12Buffer::GPUBuffer::ChengeState(
	ID3D12GraphicsCommandList* a_pCmdList, 
	const D3D12_RESOURCE_STATES& a_nextState
)
{
	// バリア
	auto _barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_cpBuffer.Get(),
		m_currentState,
		a_nextState
	);
	a_pCmdList->ResourceBarrier(1, &_barrier);

}
