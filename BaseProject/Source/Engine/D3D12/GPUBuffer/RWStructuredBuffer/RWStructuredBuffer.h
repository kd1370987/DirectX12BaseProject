#pragma once

#include "../GPUBuffer.h"

namespace Engine::D3D12
{

	/// <summary>
	/// GPU上で完結するバッファクラス
	/// CPUからは触らない
	/// </summary>
	template<typename T>
	class RWStructuredBuffer : public GPUBuffer
	{
	public:

		virtual ~RWStructuredBuffer() override = default;
		NON_COPYABLE_MOVABLE(RWStructuredBuffer);

		/// <summary>
		/// 作成
		/// </summary>
		/// <param name="a_pDevice">デバイスポインタ</param>
		/// <param name="a_elementNum">要素数</param>
		/// <param name="a_strideSize">要素サイズ</param>
		void Create(D3D12::Device* a_pDevice, UINT a_elementNum);
	};

	template<typename T>
	inline void RWStructuredBuffer<T>::Create(D3D12::Device* a_pDevice, UINT a_elementNum)
	{
		// バッファ作成
		GPUBufferDesc _desc = {};
		_desc.elementNum = a_elementNum;
		_desc.strideSize = sizeof(T);
		_desc.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		_desc.heapType = D3D12_HEAP_TYPE_DEFAULT;
		if (!GPUBuffer::Create(a_pDevice, _desc))
		{
			assert(0 && "RWStructredBufferの作成に失敗");
			return;
		}

		// 仕様書作成
		// SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		_srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_srvDesc.Buffer.FirstElement = 0;
		_srvDesc.Buffer.NumElements = a_elementNum;
		_srvDesc.Buffer.StructureByteStride = sizeof(T);
		_srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// UAV
		D3D12_UNORDERED_ACCESS_VIEW_DESC _uavDesc = {};
		_uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		_uavDesc.Format = DXGI_FORMAT_UNKNOWN;					// ストラクチャードバッファは必ずUNKNOWN
		_uavDesc.Buffer.FirstElement = 0;
		_uavDesc.Buffer.NumElements = a_elementNum;
		_uavDesc.Buffer.StructureByteStride = sizeof(T);
		_uavDesc.Buffer.CounterOffsetInBytes = 0;				// Append/Consumeは使わないので0
		_uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		// ハンドルをもらう
		m_srvHandle = AllocateSRV(a_pDevice,GetResource(), _srvDesc);
		m_uavHandle = AllocateUAV(a_pDevice, GetResource(),_uavDesc);
	}
}