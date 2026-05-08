#include "IndexBuffer.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../../DescriptorHeapManager/DescriptorHeapManager.h"

bool IndexBuffer::Create(
	size_t a_size, 
	size_t a_stride,
	const uint32_t* a_pInitData,
	DXGI_FORMAT a_format
)
{
	// バッファのサイズ
	size_t _bufferSize = a_size * a_stride;

	m_stride = a_stride;

	// インデックスバッファの生成
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	D3D12_RESOURCE_DESC _desc = CD3DX12_RESOURCE_DESC::Buffer(_bufferSize);	// リソースの設定

	// リソースを生成
	auto _hr = Engine::D3D12::D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_prop,										// ヒートプロパティ
		D3D12_HEAP_FLAG_NONE,						// ヒープフラグ
		&_desc,										// リソース記述子（サイズ、フォーマット）
		D3D12_RESOURCE_STATE_GENERIC_READ,			// 初期のリソースステート
		nullptr,									// 最適化されたクリア値（テクスチャ用,Bufferならnullptr）
		IID_PPV_ARGS(m_pBuffer.GetAddressOf())		// 出力のCOMポインタ受け取り
	);
	if (FAILED(_hr))
	{
		assert(0 && "[OnInit]　インデックスバッファのリソースの生成に失敗");
		return false;
	}

	// インデックバッファのビュー設定
	m_view = {};
	m_view.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_view.Format = a_format;
	m_view.SizeInBytes = static_cast<UINT>(_bufferSize);

	// マッピングする(GPUのVRAM上にあるリソースをCPUから直接触れるようにアドレスを紐づけること)
	if (a_pInitData != nullptr)
	{
		void* _ptr = nullptr;
		_hr = m_pBuffer->Map(0, nullptr, &_ptr);
		if (FAILED(_hr))
		{
			assert(0 && "[OnInit] インデックスバッファマッピングに失敗");
		}

		// インデックスデータをマッピング先に設定
		// CPUからGPUのバッファに直接アクセス
		memcpy(_ptr, a_pInitData, _bufferSize);

		// マッピング解除(CPUとGPUの紐づけを解除)
		m_pBuffer->Unmap(0, nullptr);
	}

	m_format = a_format;
	m_count = static_cast<UINT>(_bufferSize / ((a_format == DXGI_FORMAT_R16_UINT) ? 2 : 4));

	//CreateSRV();

	return true;
}

void IndexBuffer::CreateSRV()
{
	// 仕様書作成
	D3D12_SHADER_RESOURCE_VIEW_DESC _desc = {};
	_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	_desc.Format = DXGI_FORMAT_UNKNOWN;
	_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_desc.Buffer.FirstElement = 0;
	_desc.Buffer.NumElements = m_count;
	_desc.Buffer.StructureByteStride = m_stride;
	_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto _pDev = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
	m_srvHandle = Engine::D3D12::DescriptorHeapManager::Instance().Allocate<Engine::D3D12::SRV>(_pDev,m_pBuffer.Get(),&_desc);
}

Engine::Resource::Handle<Engine::D3D12::SRV> IndexBuffer::GetHandle() const
{
	return m_srvHandle;
}

const D3D12_INDEX_BUFFER_VIEW& IndexBuffer::View() const
{
	return m_view;
}

const D3D12_INDEX_BUFFER_VIEW* IndexBuffer::GetView() const
{
	return &m_view;
}

const D3D12_GPU_VIRTUAL_ADDRESS& IndexBuffer::GetGPUVirtualAddress() const
{
	return m_pBuffer->GetGPUVirtualAddress();
}

DXGI_FORMAT IndexBuffer::GetFormat() const
{
	return m_format;
}

bool IndexBuffer::Valid() const
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
