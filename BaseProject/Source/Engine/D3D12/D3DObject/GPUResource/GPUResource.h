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
		bool Create(ID3D12Device* pDevice ,const GPUResourceDesc& a_desc);

		// 解放
		virtual void Release();

		// ステート遷移
		virtual void Barrier(ID3D12GraphicsCommandList* a_pCmdList,D3D12_RESOURCE_STATES a_nextState);

		// アクセサ
		virtual ID3D12Resource* GetResource() const;
		virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
		size_t GetBufferSize() const;
		size_t GetStrideSize() const;
		size_t GetElementNum() const;

	protected :

		// データ
		ComPtr<ID3D12Resource> m_cpResource = nullptr;
		D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;

		size_t m_bufferSize = 0;
		size_t m_strideSize = 0;
		size_t m_elementNum = 0;
	public:

		NON_COPYABLE_MOVABLE(GPUResource);
	};
}