#pragma once
namespace Engine::Resource
{
	//==========================================================
	// ラスタライザパイプライン用
	//==========================================================
	struct RasterizationMesh
	{
		void Create(
			ID3D12Device* a_pDevice, 
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face,
			DXGI_FORMAT a_indexFormat
		);

		D3D12::DynamicVertexBuffer<MeshVertexFloat> vertexBuffer;		// 頂点バッファ
		D3D12::DynamicIndexBuffer					indexBuffer;		// インデックスバッファ
	};
}