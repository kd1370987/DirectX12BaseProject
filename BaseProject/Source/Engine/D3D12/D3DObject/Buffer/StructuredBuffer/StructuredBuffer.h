#pragma once

namespace Engine::D3D12
{
	template<typename T>
	class StructuredBuffer
	{
	public:

		StructuredBuffer() = default;
		~StructuredBuffer();

		/// <summary>
		/// 生成
		/// </summary>
		/// <param name="a_numElement">構造体数</param>
		/// <param name="a_pInitData">初期化時情報</param>
		bool Create(
			ID3D12Device* a_pDevice,
			int a_numElement,
			T* a_pInitData = nullptr
		);

		// SRV生成時用にリソースを返す
		ID3D12Resource* GetResource();


		T& RefData(uint32_t a_index);

	private:

		// バッファ情報
		ComPtr<ID3D12Resource> m_cpBuffer = nullptr;		// バッファ本体
		Engine::Resource::Handle<SRV> m_srvHandle = {};		// アロケートされた場所

		// データ
		void* m_bufferDate = nullptr;

		// 構成情報
		int m_numElement = 0;		// 要素数
		int m_sizeOfElement = 0;	// エレメントのサイズ
	};

	template<typename T>
	inline StructuredBuffer<T>::~StructuredBuffer()
	{
		if (m_bufferDate)
		{
			m_bufferDate = nullptr;
		}
	}

	template<typename T>
	inline bool StructuredBuffer<T>::Create(ID3D12Device* a_pDevice, int a_numElement, T* a_pInitData)
	{
		// サイズ
		m_sizeOfElement = sizeof(T);	// １要素当たりのサイズ
		m_numElement = a_numElement;	// 要素数
		size_t _totalSize = m_sizeOfElement * m_numElement;		// バッファ全体のサイズ

		// 初期化情報
		auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_CUSTOM);
		auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_totalSize);

		// バッファの作成
		HRESULT _hr = a_pDevice->CreateCommittedResource(
			&_prop,
			D3D12_HEAP_FLAG_NONE,
			&_desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(m_cpBuffer.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			assert(0 && "構造体バッファの生成に失敗");
			return false;
		}

		return true;
	}
	template<typename T>
	inline ID3D12Resource* StructuredBuffer<T>::GetResource()
	{
		return m_cpBuffer.Get();
	}
	template<typename T>
	inline T& StructuredBuffer<T>::RefData(uint32_t a_index)
	{
		return reinterpret_cast<T>(m_bufferDate[a_index]);
	}
}