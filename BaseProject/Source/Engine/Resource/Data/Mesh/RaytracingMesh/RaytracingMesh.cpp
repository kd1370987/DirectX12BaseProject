#include "RaytracingMesh.h"

#include "../../../../MainEngine.h"
#include "../../../../Graphics/GraphicEngine.h"

namespace Engine::Resource
{
	void RaytracingMesh::Create(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList,
		const std::vector<MeshSubset>& a_subset,
		const D3D12::DynamicVertexBuffer<MeshVertexFloat>& a_vertexBuffer,
		DXGI_FORMAT a_vertexFarstFormat,
		const D3D12::DynamicIndexBuffer& a_indexBuffer,						// インデックスバッファ
		const std::vector<MeshVertexFloat>& a_vertices,
		const std::vector<MeshFace>& a_face
	)
	{
		// BLAS構築
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> _descVec;
		// レイトレーシング用データ作成
		for (auto& _subset : a_subset)
		{
			// ジオメトリ記述作成
			D3D12_RAYTRACING_GEOMETRY_DESC _desc = {};
			_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			// 頂点バッファ
			_desc.Triangles.VertexBuffer.StartAddress = a_vertexBuffer.GetGPUVirtualAddress();
			_desc.Triangles.VertexBuffer.StrideInBytes = a_vertexBuffer.GetStrideSize();
			_desc.Triangles.VertexCount = a_vertexBuffer.GetElementNum();
			_desc.Triangles.VertexFormat = a_vertexFarstFormat;

			// インデックスバッファ
			_desc.Triangles.IndexBuffer = a_indexBuffer.GetGPUVirtualAddress() + sizeof(UINT) * _subset.faceStart * 3;
			_desc.Triangles.IndexCount = _subset.faceCount * 3;
			_desc.Triangles.IndexFormat = a_indexBuffer.GetView().Format;

			_descVec.push_back(_desc);
		}

		// BLAS作成
		blas.CreateStatic(a_pDevice, a_pCmdList, _descVec);

		return;
	}
	void RaytracingMesh::Release()
	{
		blas.Release();
	}
}