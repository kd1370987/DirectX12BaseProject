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
		GPUBuffer() = default;
		virtual ~GPUBuffer() override = default;
		NON_COPYABLE_MOVABLE(GPUBuffer);

		// バッファ専用の作成
		bool Create(D3D12::Device* a_pDevice,const GPUBufferDesc& a_desc);

		// データの書き込み(アップロードヒープ用)
		void Map(void** a_ppData);
		void Unmap();

		// 一回限りの書きこみ
		void Write(const void* a_pData,size_t a_size);

	protected:
		Handle<SRV> AllocateSRV(D3D12::Device* a_pDevice,ID3D12Resource* a_pRes,const D3D12_SHADER_RESOURCE_VIEW_DESC& a_desc);
		Handle<UAV> AllocateUAV(D3D12::Device* a_pDevice,ID3D12Resource* a_pRes,const D3D12_UNORDERED_ACCESS_VIEW_DESC& a_desc);
	};
}