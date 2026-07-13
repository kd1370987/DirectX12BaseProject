#include "RaytracingMesh.h"

#include "../../../../MainEngine.h"
#include "../../../../Graphics/GraphicEngine.h"
#include "../../../../Graphics/MeshBufferAllocator/MeshBufferAllocator.h"

namespace Engine::Resource
{
	void RaytracingMesh::Create(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList,
		const std::vector<MeshSubset>& a_subset
	)
	{
		// メッシュのバッファを取得
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		auto* _pMA = _pGE->RefMeshBufferAllocator();

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
			_desc.Triangles.VertexBuffer.StartAddress = 
				_pMA->RefStaticVertexBuffer().GetGPUVirtualAddress() + 
				(vertexHandle.startIndex) * sizeof(MeshVertexFloat);
			_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(MeshVertexFloat);
			_desc.Triangles.VertexCount = vertexHandle.count;
			_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

			// インデックスバッファ
			_desc.Triangles.IndexBuffer 
				= _pMA->RefIndexBuffer().GetGPUVirtualAddress() + (indexHandle.startIndex + _subset.faceStart * 3) * sizeof(UINT);
			_desc.Triangles.IndexCount = _subset.faceCount * 3;
			_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

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