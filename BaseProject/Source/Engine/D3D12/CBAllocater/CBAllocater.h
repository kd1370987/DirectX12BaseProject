#pragma once

#include "Engine/D3D12//D3DObject/DescriptorHeap/CBV_SRV_UAVHeap/CBV_SRV_UAVHeap.h"

class CBAllocater
{
public:
	
	void RootCBVCreate(ID3D12Device* a_device, size_t a_memSize);
	
	// 使用リセット
	void ResetUse()
	{
		m_usedCount = 0;
	}

	// データをバインドして転送
	template<typename T>
	void BindAndAttachDataRootCBV(ID3D12GraphicsCommandList* a_pCmdList, int a_descIndex, const T& a_data);

private:
	UINT m_usedCount = 0;
	CBV_SRV_UAVHeap* m_pHeap = nullptr;
	ID3D12Device* m_pDevice = nullptr;
	ComPtr<ID3D12Resource> m_spResource = nullptr;
	struct { uint8_t buff[256]; }*m_pMappedData = nullptr;

	size_t m_capacity = 0;
};

template<typename T>
inline void CBAllocater::BindAndAttachDataRootCBV(
	ID3D12GraphicsCommandList* a_pCmdList,
	int a_descIndex,
	const T& a_data
)
{
	size_t _dataSize = (sizeof(T) + 0xff) & ~0xff; // 256バイトアライメント

	int _useValue = _dataSize / 0x100;
	if ((m_usedCount + _useValue) * 256 > m_capacity)
	{
		// ヒープに登録できる数を超えた
		assert(0 && "アップロードヒープの上限を迎えました");
		return;
	}

	// アドレス位置
	int _top = m_usedCount;

	// データ転送
	std::memcpy(&m_pMappedData[_top].buff, &a_data, sizeof(T));

	// コマンドリストにセット
	a_pCmdList->SetGraphicsRootConstantBufferView(
		a_descIndex,
		m_spResource->GetGPUVirtualAddress() + ((UINT64)_top * 0x100)
	);

	m_usedCount += _useValue;
}