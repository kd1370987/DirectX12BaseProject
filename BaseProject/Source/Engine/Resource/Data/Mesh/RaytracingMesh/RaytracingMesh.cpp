#include "RaytracingMesh.h"

#include "../../../../D3D12/D3DObject/CommandList/CommandList.h"

namespace Engine::Resource
{
	void RaytracingMesh::Create(
		ID3D12Device* a_pDevice,
		D3D12::CommandList* a_pCmdList,
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
		blas.Create(_descVec);

		// 構造体バッファ作成
		std::vector<RTVertex> _rtVertDataVec = {};
		for (auto& _vert : a_vertices)
		{
			RTVertex _rt = {};
			_rt = _vert;
			_rtVertDataVec.push_back(_rt);
		}
		// 頂点バッファー側SRV作成
		structuredVertexBuffer.Create(a_pDevice, *a_pCmdList, _rtVertDataVec.size(), _rtVertDataVec.data());

		std::vector<UINT> _indices;		// インデックス配列作成
		for (auto& _f : a_face)
		{
			_indices.push_back(_f.idx[0]);
			_indices.push_back(_f.idx[1]);
			_indices.push_back(_f.idx[2]);
		}
		// インデックスバッファー側SRV作成
		structuredIndexBuffer.Create(a_pDevice, *a_pCmdList, _indices.size(), _indices.data());
	}
	void RaytracingMesh::Release()
	{
		blas.Release();
		structuredIndexBuffer.Release();
		structuredVertexBuffer.Release();
	}
}