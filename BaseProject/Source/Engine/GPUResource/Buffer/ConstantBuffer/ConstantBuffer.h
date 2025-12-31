#pragma once


class ConstantBuffer
{
public:
	// コンストラクタで定数バッファを生成
	ConstantBuffer(ID3D12Device* a_pDevice);

	bool Create(size_t a_size);

	void* GetPtr() const;								// 定数バッファにマッピングされたポインタを返す

	template<typename T>
	T* GetPtr()
	{
		return reinterpret_cast<T*>(GetPtr());
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetHandle() { return m_handle; }

	const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetCBVDesc() const { return m_cbvDesc; }

private:

	// デバイス
	ID3D12Device* m_pDevice = nullptr;

	// 定数バッファ
	ComPtr<ID3D12Resource> m_cpBuffer = nullptr;
	void* m_pMappedPtr = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE m_handle;

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc;

	// コピー禁止
	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator = (const ConstantBuffer&) = delete;
};
