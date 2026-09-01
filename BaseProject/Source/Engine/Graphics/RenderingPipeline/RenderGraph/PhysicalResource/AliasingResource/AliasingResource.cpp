#include "AliasingResource.h"
namespace Engine::Graphics::Pipeline
{
	void AliasingResource::Create(D3D12::Device* a_pDevice, ID3D12Heap* a_pHeap, UINT64 a_width, UINT a_height, DXGI_FORMAT a_maxFormat)
	{
		// リソース作成
		D3D12_RESOURCE_DESC _resDesc = {};
		_resDesc.Format = a_maxFormat;
		_resDesc.Width = a_width;
		_resDesc.Height = a_height;
		_resDesc.MipLevels = 1;
		_resDesc.DepthOrArraySize = 1;
		_resDesc.SampleDesc.Count = 1;
		_resDesc.SampleDesc.Quality = 0;
		_resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		_resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		// 

	}
}