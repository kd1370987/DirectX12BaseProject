#pragma once

#include "../DynamicBuffer/DynamicBuffer.h"

namespace Engine::D3D12
{
	struct ByteAddressBufferDesc
	{
		const void* pData = nullptr;
		uint32_t count = 0;
		size_t strideSize = 0;
	};

	/// <summary>
	/// シェーダーからバイトでアクセスされるシェーダー
	/// </summary>
	class ByteAddressBuffer : public DynamicBuffer
	{
	public:

		~ByteAddressBuffer() override = default;
		NON_COPYABLE_MOVABLE(ByteAddressBuffer);

		// 作成
		bool Create(D3D12::Device* a_pDevice, const ByteAddressBufferDesc& a_desc);

		// アクセサ
		const Handle<SRV>& GetSRVHandle() const;

	};
}

