#include "Engine/Graphics/D3D12/GPUBuffer/ByteAddressBuffer/StaticByteAddressBuffer.h"
namespace Engine::Graphics::D3D12
{
	bool StaticByteAddressBuffer::Create(Graphics::D3D12::Device* a_pDevice, DescriptorHeapManager* a_pHeapManager, Graphics::D3D12::GraphicsCommandList* a_pCmdList, UINT a_elementNum, size_t a_strideSize, const void* a_pData)
	{
		StaticBufferDesc _desc = {};
		_desc.elementNum = a_elementNum;
		_desc.strideSize = a_strideSize;
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;
		if (!StaticBuffer::Create(a_pDevice, a_pHeapManager, a_pCmdList, _desc, a_pData))
		{
			ENGINE_ERRLOG(false, "ストラクチャバッファの生成に失敗");
			return false;
		}

		// ビュー作成
		m_view = {};
		m_view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		m_view.Format = DXGI_FORMAT_UNKNOWN;
		m_view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_view.Buffer.FirstElement = 0;
		m_view.Buffer.NumElements = static_cast<UINT>(m_elementNum);
		m_view.Buffer.StructureByteStride = static_cast<UINT>(a_strideSize);
		m_view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// SRV作成
		m_srvHandle = AllocateSRV(a_pDevice,a_pHeapManager,GetResource(),m_view);

		return true;
	}
	void StaticByteAddressBuffer::UploadDataRange(Graphics::D3D12::GraphicsCommandList* a_pCmdList, UINT a_startIndex, UINT a_count, const void* a_pData)
	{
		StaticBuffer::UploadDataRange(
			a_pCmdList,
			a_startIndex * m_strideSize,
			a_pData,
			a_count * m_strideSize
		);
	}
}