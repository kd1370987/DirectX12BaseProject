#include "GraphHeap.h"
namespace Engine::Graphics::Pipeline
{
	void GraphHeap::Create(D3D12::Device* a_pDevice, UINT64 a_maxWidth, UINT a_maxHeight)
	{
		m_width = a_maxWidth;
		m_height = a_maxHeight;

		// もっとも大きなテクスチャ情報を作成
		D3D12_RESOURCE_DESC _texDesc = {};
		_texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		_texDesc.Width = m_width;
		_texDesc.Height = m_height;
		_texDesc.MipLevels = 1;
		_texDesc.DepthOrArraySize = 1;
		_texDesc.SampleDesc.Count = 1;
		_texDesc.SampleDesc.Quality = 0;
		_texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		_texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		// ヒープを作成
		D3D12_RESOURCE_ALLOCATION_INFO _info = a_pDevice->GetResourceAllocationInfo(0, 1, &_texDesc);
		CD3DX12_HEAP_DESC _heapDesc(_info, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE);
		a_pDevice->CreateHeap(&_heapDesc, IID_PPV_ARGS(&m_cpHeap));
	}
}