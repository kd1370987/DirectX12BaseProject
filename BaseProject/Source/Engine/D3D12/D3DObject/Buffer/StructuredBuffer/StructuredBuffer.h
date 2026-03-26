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
		D3D12_SHADER_RESOURCE_VIEW_DESC* GetViewDesc();

		// SRVをセット
		void SetHandle(const Engine::Resource::Handle<SRV>& a_handle)
		{
			m_srvHandle = a_handle;
		}
		const Engine::Resource::Handle<SRV>& GetHandle() const
		{
			return m_srvHandle;
		}

		T& RefData(uint32_t a_index);

		// 更新
		void Update(ID3D12Device* a_pDevice, ID3D12GraphicsCommandList* a_pCmdList);

	private:

		// GPUバッファを作成
		void CreateCopyBuffer(ID3D12Device* a_pDevice,size_t a_memSize);

		// GPUバッファにコピー
		void CopyToGPU(ID3D12Device* a_pDevice, ID3D12GraphicsCommandList* a_pCmdList);

	private:

		// バッファ情報
		ComPtr<ID3D12Resource> m_cpUploadBuffer = nullptr;		// バッファ本体

		ComPtr<ID3D12Resource> m_cpGPUBuffer = nullptr;		// コピー用
		Engine::Resource::Handle<SRV> m_srvHandle = {};		// アロケートされた場所
		size_t m_maxCopyMemSize = 0;

		// データ
		void* m_bufferDate = nullptr;

		// 構成情報
		int m_numElement = 0;		// 要素数
		int m_sizeOfElement = 0;	// エレメントのサイズ
		size_t m_totalSize = 0;		// バッファサイズ

		// 操作があったかどうか
		bool m_isDarty = false;

		// ビュー構造体
		D3D12_SHADER_RESOURCE_VIEW_DESC m_viewDesc = {};
	};

	template<typename T>
	inline StructuredBuffer<T>::~StructuredBuffer()
	{
		if (m_cpUploadBuffer)
		{
			m_cpUploadBuffer->Unmap(0,nullptr);
		}
	}

	template<typename T>
	inline bool StructuredBuffer<T>::Create(ID3D12Device* a_pDevice, int a_numElement, T* a_pInitData)
	{
		// サイズ
		m_sizeOfElement = sizeof(T);	// １要素当たりのサイズ
		m_numElement = a_numElement;	// 要素数
		size_t _totalSize = m_sizeOfElement * m_numElement;		// バッファ全体のサイズ
		m_totalSize = _totalSize;


		// 初期化情報
		auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_totalSize);

		// バッファの作成
		HRESULT _hr = a_pDevice->CreateCommittedResource(
			&_prop,
			D3D12_HEAP_FLAG_NONE,
			&_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_cpUploadBuffer.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			assert(0 && "構造体バッファの生成に失敗");
			return false;
		}

		// マップ
		m_cpUploadBuffer->Map(0,nullptr,&m_bufferDate);
		// 初期データがあれば上書き
		if (a_pInitData)
		{
			memcpy(m_bufferDate,a_pInitData,_totalSize);
		}

		// GPUバッファ作成
		CreateCopyBuffer(a_pDevice,_totalSize);

		m_isDarty = true;

		// ビュー構造体作成
		m_viewDesc = {};
		m_viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		m_viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		m_viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_viewDesc.Buffer.FirstElement = 0;
		m_viewDesc.Buffer.NumElements = m_numElement;
		m_viewDesc.Buffer.StructureByteStride = sizeof(T);
		m_viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		return true;
	}
	template<typename T>
	inline ID3D12Resource* StructuredBuffer<T>::GetResource()
	{
		return m_cpGPUBuffer.Get();
	}
	template<typename T>
	inline D3D12_SHADER_RESOURCE_VIEW_DESC* StructuredBuffer<T>::GetViewDesc()
	{
		return &m_viewDesc;
	}
	template<typename T>
	inline T& StructuredBuffer<T>::RefData(uint32_t a_index)
	{
		m_isDarty = true;
		return reinterpret_cast<T*>(m_bufferDate)[a_index];
	}
	template<typename T>
	inline void StructuredBuffer<T>::Update(ID3D12Device* a_pDevice, ID3D12GraphicsCommandList* a_pCmdList)
	{
		if (m_isDarty)
		{
			CopyToGPU(a_pDevice,a_pCmdList);

			m_isDarty = false;
		}
	}
	template<typename T>
	inline void StructuredBuffer<T>::CreateCopyBuffer(ID3D12Device* a_pDevice, size_t a_memSize)
	{
		// リソース作成情報
		auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto _desc = CD3DX12_RESOURCE_DESC::Buffer(a_memSize);

		// リソース作成
		a_pDevice->CreateCommittedResource(
			&_prop,
			D3D12_HEAP_FLAG_NONE,
			&_desc,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&m_cpGPUBuffer)
		);

		// サイズ記憶
		m_maxCopyMemSize = a_memSize;
	}
	template<typename T>
	inline void StructuredBuffer<T>::CopyToGPU(ID3D12Device* a_pDevice, ID3D12GraphicsCommandList* a_pCmdList)
	{
		// バリア
		auto _barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_cpGPUBuffer.Get(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		a_pCmdList->ResourceBarrier(1, &_barrier);

		// コピー
		a_pCmdList->CopyBufferRegion(
			m_cpGPUBuffer.Get(),
			0,
			m_cpUploadBuffer.Get(),
			0,
			m_totalSize
		);

		// バリア
		_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_cpGPUBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		a_pCmdList->ResourceBarrier(1,&_barrier);
	}
}