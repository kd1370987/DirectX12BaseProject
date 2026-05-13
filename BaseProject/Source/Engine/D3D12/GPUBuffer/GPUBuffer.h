#pragma once

#include "../D3DObject/GPUResource/GPUResource.h"

namespace Engine::D3D12
{
	struct GPUBufferDesc
	{
		size_t strideSize = 0;
		size_t elementNum = 0;
		D3D12_HEAP_TYPE heapType;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	};

	/// GPUバッファの基底クラス
	class GPUBuffer : public GPUResource
	{
	public:
		virtual ~GPUBuffer() override = default;

		// バッファ専用の作成
		bool Create(ID3D12Device* a_pDevice,const GPUBufferDesc& a_desc);

		// データの書き込み(アップロードヒープ用)
		void Map(void** a_ppData);
		void Unmap();

		// 一回限りの書きこみ
		void Write(const void* a_pData,size_t a_size);
	protected:

		// SRV作成関数
		void CreateSRVInternal(ID3D12Device* a_pDevice);

	protected:

		Resource::Handle<D3D12::SRV> m_srvHandle = {};

	};
}