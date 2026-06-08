#pragma once
namespace Engine::Resource
{
	//==========================================================
	// レイトレ用データ
	//==========================================================
	struct RaytracingMesh
	{
		void Create(
			ID3D12Device* a_pDevice,
			D3D12::CommandList* a_pCmdList,
			const std::vector<MeshSubset>& a_subset,							// サブセット配列
			const D3D12::DynamicVertexBuffer<MeshVertexFloat>& a_vertexBuffer,	// 頂点バッファ
			DXGI_FORMAT a_vertexFarstFormat,
			const D3D12::DynamicIndexBuffer& a_indexBuffer,						// インデックスバッファ
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face
		);

		// 解放
		void Release();

		Engine::Raytracing::BLAS blas;
		Engine::D3D12::StaticStructuredBuffer<RTVertex> structuredVertexBuffer;
		Engine::D3D12::StaticStructuredBuffer<UINT>		structuredIndexBuffer;
	};
}