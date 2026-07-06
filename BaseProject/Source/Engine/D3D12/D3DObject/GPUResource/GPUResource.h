#pragma once
namespace Engine::D3D12
{
	// 前方宣言
	class CommandList;

	// リソース作成用構造体
	struct GPUResourceDesc
	{
		D3D12_HEAP_TYPE		heapType	= D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC resourceDesc = {};	// リソース作成情報
		size_t				strideSize = 0;		// 一つのサイズ
		size_t				elementNum = 0;		// 要素数

		D3D12_HEAP_FLAGS		heapFlags	= D3D12_HEAP_FLAG_NONE;
		D3D12_RESOURCE_STATES	farstState	= D3D12_RESOURCE_STATE_COMMON;
		DXGI_FORMAT				format		= DXGI_FORMAT_UNKNOWN;
		D3D12_CLEAR_VALUE*		pClearValue	= nullptr;						// バッファだとnullptrなので

		// 用途フラグ
		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	};

	// リソース規定クラス
	class GPUResource
	{
	public:

		virtual ~GPUResource() { Release(); }

		// 作成
		bool Create(D3D12::Device* pDevice ,const GPUResourceDesc& a_desc);

		// 解放
		virtual void Release();

		// ステート遷移
		virtual void Barrier(D3D12::GraphicsCommandList* a_pCmdList,D3D12_RESOURCE_STATES a_nextState);

		// アクセサ
		virtual ID3D12Resource* GetResource() const;
		virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
		size_t GetBufferSize() const;
		size_t GetStrideSize() const;
		size_t GetElementNum() const;
		D3D12_RESOURCE_STATES GetState() const { return m_currentState; }

		// =========================================================
		// ハンドルのアクセサ
		// =========================================================
		const Handle<D3D12::SRV>& GetSRV() const { return m_srvHandle; }
		const Handle<D3D12::UAV>& GetUAV() const { return m_uavHandle; }
		const Handle<D3D12::RTV>& GetRTV() const { return m_rtvHandle; }
		const Handle<D3D12::DSV>& GetDSV() const { return m_dsvHandle; }
		const Handle<D3D12::DSV>& GetReadOnlyDSV() const { return m_readOnlyDsvHandle; }
		const Handle<D3D12::SRV>& GetImGuiSRV() const { return m_imguiSRVHandle; }

	protected:

		// データ
		ComPtr<ID3D12Resource> m_cpResource = nullptr;
		D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;

		size_t m_bufferSize = 0;
		size_t m_strideSize = 0;
		size_t m_elementNum = 0;

		// ビューハンドル
		Handle<D3D12::SRV> m_srvHandle = {};
		Handle<D3D12::UAV> m_uavHandle = {};
		Handle<D3D12::RTV> m_rtvHandle = {};
		Handle<D3D12::DSV> m_dsvHandle = {};
		Handle<D3D12::DSV> m_readOnlyDsvHandle = {};
		Handle<D3D12::SRV> m_imguiSRVHandle = {};
	};
}