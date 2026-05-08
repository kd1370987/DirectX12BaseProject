#include "VertexBuffer.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../../DescriptorHeapManager/DescriptorHeapManager.h"

bool VertexBuffer::Create(
	size_t a_size,
	size_t a_stride, 
	const void* a_pInitData
)
{
	// 頂点バッファの生成
	size_t _bufferSize = a_size * a_stride;

	m_vertexCount = a_size;
	m_strideSize = a_stride;

	// 初期化情報
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_bufferSize);					// リソースの設定

	// リソースの生成
	auto _hr = Engine::D3D12::D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pBuffer.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "頂点バッファリソースの生成に失敗");
		return false;
	}

	// 頂点バッファビューの設定
	m_view.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_view.SizeInBytes = static_cast<UINT>(_bufferSize);
	m_view.StrideInBytes = static_cast<UINT>(a_stride);

	// マッピングする
	if (a_pInitData != nullptr)
	{
		void* _ptr = nullptr;
		_hr = m_pBuffer->Map(0, nullptr, &_ptr);
		if (FAILED(_hr))
		{
			assert(0 && "頂点バッファマッピングに失敗");
			return false;
		}

		// 頂点データをマッピング先に設定
		memcpy(_ptr, a_pInitData, _bufferSize);

		// マッピング解除
		m_pBuffer->Unmap(0, nullptr);
	}



	// 作成成功
	return true;
}

void VertexBuffer::CreateSRV()
{
	// 仕様書作成
	D3D12_SHADER_RESOURCE_VIEW_DESC _desc = {};
	_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	_desc.Format = DXGI_FORMAT_UNKNOWN;
	_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_desc.Buffer.FirstElement = 0;
	_desc.Buffer.NumElements = m_vertexCount;
	_desc.Buffer.StructureByteStride = m_strideSize;
	_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto _pDev = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
	m_srvHandle = Engine::D3D12::DescriptorHeapManager::Instance().Allocate<Engine::D3D12::SRV>(_pDev, m_pBuffer.Get(), &_desc);
}

Engine::Resource::Handle<Engine::D3D12::SRV> VertexBuffer::GetHandle() const
{
	return m_srvHandle;
}

void VertexBuffer::Update(size_t a_count, const void* a_data)
{
	// バッファーサイズ
	size_t _bufferSize = a_count * m_view.StrideInBytes;

	// マップして更新
	void* _ptr = nullptr;
	m_pBuffer->Map(0,nullptr,&_ptr);

	std::memcpy(_ptr,a_data,_bufferSize);

	m_pBuffer->Unmap(0, nullptr);

	// ビュー情報修正
	m_view.SizeInBytes = static_cast<UINT>(_bufferSize);
}

const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer::View() const
{
	return m_view;
}

const D3D12_VERTEX_BUFFER_VIEW* VertexBuffer::GetView() const
{
	return &m_view;
}

const D3D12_GPU_VIRTUAL_ADDRESS& VertexBuffer::GetGPUVirtualAddress() const
{
	return m_pBuffer->GetGPUVirtualAddress();
}

UINT64 VertexBuffer::GetStrideSize() const
{
	return static_cast<UINT64>(m_strideSize);
}

UINT VertexBuffer::GetCount() const
{
	return static_cast<UINT>(m_vertexCount);
}

bool VertexBuffer::Valid() const
{
	if (m_pBuffer)
	{
		return true;
	}
	else
	{
		return false;
	}
}
