#pragma once

#include "Engine/GPUResource/DescriptorHeap/CBV_SRV_UAVHeap/CBV_SRV_UAVHeap.h"

class CBAllocater
{
public:
	void Init(ID3D12Device* a_device);
	
	// 使用リセット
	void ResetUse()
	{
		m_usedCount = 0;
	}

	// データをバインドして転送
	template<typename T>
	void BindAndAttachData(ID3D12GraphicsCommandList* a_pCmdList,int a_descIndex, const T& a_data);

private:
	UINT m_usedCount = 0;
	CBV_SRV_UAVHeap* m_pHeap = nullptr;
	ID3D12Device* m_pDevice = nullptr;
	ComPtr<ID3D12Resource> m_spResource = nullptr;
	struct { uint8_t buff[256]; }*m_pMappedData = nullptr;
};

template<typename T>
inline void CBAllocater::BindAndAttachData(
	ID3D12GraphicsCommandList* a_pCmdList,
	int a_descIndex, 
	const T& a_data
)
{
	if(!m_pHeap) return;

	
	size_t _dataSize = (1024 + 0xff) & ~0xff; // 256バイトアライメント
	int _useValue = _dataSize / 0x100;
	if (m_usedCount + _useValue > m_pHeap->GetMaxCounts().x)
	{
		// ヒープに登録できる数を超えた
		return;
	}

	// アドレス位置
	int _top = m_usedCount;

	// データ転送
	std::memcpy(&m_pMappedData[_top].buff, &a_data, sizeof(T));


	// ビューを作って、値をシェーダーにアタッチ
	D3D12_CONSTANT_BUFFER_VIEW_DESC _cbvDesc = {};
	_cbvDesc.BufferLocation = m_spResource->GetGPUVirtualAddress() + ((UINT64)_top * 0x100);
	_cbvDesc.SizeInBytes = static_cast<UINT>(_dataSize);

	D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle = m_pHeap->GetCPUHandle(_top);

	m_pDevice->CreateConstantBufferView(&_cbvDesc, _cpuHandle);

	D3D12_GPU_DESCRIPTOR_HANDLE _gpuHandle = m_pHeap->GetGPUHandle(_top);

	// コマンドリストにセット
	a_pCmdList->SetGraphicsRootDescriptorTable(
		a_descIndex,
		_gpuHandle
	);

	m_usedCount += _useValue;
}