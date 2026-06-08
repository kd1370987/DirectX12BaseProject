#include "RasterizationMesh.h"
namespace Engine::Resource
{
	void Engine::Resource::RasterizationMesh::Create(
		ID3D12Device* a_pDevice,
		const std::vector<MeshVertexFloat>& a_vertices,
		const std::vector<MeshFace>& a_face, 
		DXGI_FORMAT a_indexFormat
	)
	{
		// 頂点バッファ作成
		if (!vertexBuffer.CreateAndUpload(
			a_pDevice,
			(UINT)a_vertices.size(),
			a_vertices.data()
		))
		{
			assert(0 && "頂点バッファの生成に失敗");
			return;
		}

		// インデックスバッファ作成
		if (a_face.size() <= 0) return;
		std::vector<UINT> _indices = {};
		for (auto& _f : a_face)
		{
			_indices.push_back(_f.idx[0]);
			_indices.push_back(_f.idx[1]);
			_indices.push_back(_f.idx[2]);
		}

		D3D12::IndexBufferDesc _desc = {};
		_desc.count = _indices.size();
		_desc.pData = _indices.data();
		_desc.format = a_indexFormat;
		if (!indexBuffer.Create(a_pDevice, _desc))
		{
			assert(0 && "インデックスバッファの生成に失敗");
			return;
		}
	}
	void RasterizationMesh::Release()
	{
		vertexBuffer.Release();
		indexBuffer.Release();
	}
}