#pragma once


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

	// データをバインドして転送
	template<typename T>
	void BindAndAttachDataComputeRootCBV(ID3D12GraphicsCommandList* a_pCmdList, int a_descIndex, const T& a_data);

	// 登録済みの構造体のみ
	template<RootSigSemantic s>
	void BindSemanticCBV(ID3D12GraphicsCommandList* a_pCmdList, int a_regiIdx, const typename RootSemanticTraits<s>::Type& a_data);

private:

	// コンピュート用リソースデータ作成
	void CreateCompute(size_t a_memSize);

private:
	// デバイス
	ID3D12Device* m_pDevice = nullptr;

	// グラフィック用
	UINT m_usedCount = 0;
	ComPtr<ID3D12Resource> m_spResource = nullptr;
	struct { uint8_t buff[256]; }*m_pMappedData = nullptr;
	size_t m_capacity = 0;

	// コンピュート用
	UINT m_useComputeCount = 0;
	ComPtr<ID3D12Resource> m_spComputeResource = nullptr;
	struct { uint8_t buff[256]; }*m_pComputeMappedData = nullptr;
	size_t m_computeCapacity = 0;
};

template<typename T>
inline void CBAllocater::BindAndAttachDataRootCBV(
	ID3D12GraphicsCommandList* a_pCmdList,
	int a_descIndex,
	const T& a_data
)
{
	size_t _dataSize = (sizeof(T) + 0xff) & ~0xff; // 256バイトアライメント

	int _useValue = static_cast<int>(_dataSize / 0x100);
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

template<typename T>
inline void CBAllocater::BindAndAttachDataComputeRootCBV(ID3D12GraphicsCommandList* a_pCmdList, int a_regiIdx, const T& a_data)
{
	size_t _dataSize = (sizeof(T) + 0xff) & ~0xff; // 256バイトアライメント

	int _useValue = static_cast<int>(_dataSize / 0x100);
	if ((m_useComputeCount + _useValue) * 256 > m_computeCapacity)
	{
		// ヒープに登録できる数を超えた
		assert(0 && "アップロードヒープの上限を迎えました");
		return;
	}

	// アドレス位置
	int _top = m_useComputeCount;

	// データ転送
	std::memcpy(&m_pComputeMappedData[_top].buff, &a_data, sizeof(T));

	// コマンドリストにセット
	a_pCmdList->SetComputeRootConstantBufferView(
		a_regiIdx,
		m_spComputeResource->GetGPUVirtualAddress() + ((UINT64)_top * 0x100)
	);

	m_useComputeCount += _useValue;
}

template<RootSigSemantic s>
inline void CBAllocater::BindSemanticCBV(ID3D12GraphicsCommandList* a_pCmdList, int a_regiIdx, const typename RootSemanticTraits<s>::Type& a_data)
{
	size_t _dataSize = (sizeof(typename RootSemanticTraits<s>::Type) + 0xff) & ~0xff; // 256バイトアライメント

	int _useValue = static_cast<int>(_dataSize / 0x100);
	if ((m_usedCount + _useValue) * 256 > m_capacity)
	{
		// ヒープに登録できる数を超えた
		assert(0 && "アップロードヒープの上限を迎えました");
		return;
	}

	// アドレス位置
	int _top = m_usedCount;

	// データ転送
	std::memcpy(&m_pMappedData[_top].buff, &a_data, sizeof(typename RootSemanticTraits<s>::Type));

	// コマンドリストにセット
	a_pCmdList->SetGraphicsRootConstantBufferView(
		a_regiIdx,
		m_spResource->GetGPUVirtualAddress() + ((UINT64)_top * 0x100)
	);

	m_usedCount += _useValue;
}
