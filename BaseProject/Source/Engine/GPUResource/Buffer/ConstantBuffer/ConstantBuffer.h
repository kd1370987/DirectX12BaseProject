#pragma once

class ConstantBuffer
{
public:
	// コンストラクタで定数バッファを生成
	//ConstantBuffer(size_t a_size);
	ConstantBuffer();

	void Create(size_t a_size);

	bool IsValid();										// バッファの生成に成功したか
	D3D12_GPU_VIRTUAL_ADDRESS GetAddres() const;		// バッファのGPU上のアドレスを返す
	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc();			// 定数バッファビューを返す
	

	void* GetPtr() const;								// 定数バッファにマッピングされたポインタを返す

	template<typename T>
	T* GetPtr()
	{
		return reinterpret_cast<T*>(GetPtr());
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetHandle() { return m_handle; }

	void UnMap();

private:
	bool m_isValid = false;					// 定数バッファ生成に成功したか
	ComPtr<ID3D12Resource> m_pBuffer;		// 定数バッファ
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc;	// 定数バッファビューの設定
	void* m_pMappedPtr = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE m_handle;

	// コピー禁止
	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator = (const ConstantBuffer&) = delete;
};
