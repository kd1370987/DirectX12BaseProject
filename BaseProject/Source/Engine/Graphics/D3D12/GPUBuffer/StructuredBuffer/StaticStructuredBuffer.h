#pragma once

#include "Engine/Graphics/D3D12/GPUBuffer/StaticBuffer/StaticBuffer.h"
#include "Core/Debug/DebugLog.h"

namespace Engine::Graphics::D3D12
{
	template<typename T>
	class StaticStructuredBuffer : public StaticBuffer
	{
	public:
		StaticStructuredBuffer() = default;
		~StaticStructuredBuffer() override = default;
		NON_COPYABLE_MOVABLE(StaticStructuredBuffer);

		// 作成
		void Create(Graphics::D3D12::Device* a_pDevice, DescriptorHeapManager* a_pHeapManager, Graphics::D3D12::GraphicsCommandList* a_pCmdList,UINT a_elementNum,const T* a_pInitData);

		// アクセサ
		const D3D12_SHADER_RESOURCE_VIEW_DESC& GetView() const;
		const Handle<Graphics::D3D12::SRV>& GetSRVHandle() const;

	private:

		D3D12_SHADER_RESOURCE_VIEW_DESC m_view = {};
	};
	template<typename T>
	inline void StaticStructuredBuffer<T>::Create(Graphics::D3D12::Device* a_pDevice, DescriptorHeapManager* a_pHeapManager, Graphics::D3D12::GraphicsCommandList* a_pCmdList, UINT a_elementNum, const T* a_pInitData)
	{
		StaticBufferDesc _desc = {};
		_desc.elementNum = a_elementNum;
		_desc.strideSize = sizeof(T);
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;
		if (!StaticBuffer::Create(a_pDevice, a_pHeapManager, a_pCmdList, _desc, a_pInitData))
		{
			ENGINE_ERRLOG(false, "ストラクチャバッファの生成に失敗");
			return;
		}

		// ビュー作成
		m_view = {};
		m_view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		m_view.Format = DXGI_FORMAT_UNKNOWN;
		m_view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_view.Buffer.FirstElement = 0;
		m_view.Buffer.NumElements = static_cast<UINT>(m_elementNum);
		m_view.Buffer.StructureByteStride = sizeof(T);
		m_view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// SRV作成
		StaticBuffer::CreateSRVInternal(a_pDevice, a_pHeapManager);
	}
	template<typename T>
	inline const D3D12_SHADER_RESOURCE_VIEW_DESC& StaticStructuredBuffer<T>::GetView() const
	{
		return m_view;
	}
	template<typename T>
	inline const Handle<Graphics::D3D12::SRV>& StaticStructuredBuffer<T>::GetSRVHandle() const
	{
		return m_srvHandle;
	}
}