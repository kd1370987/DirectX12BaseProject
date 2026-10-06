#pragma once

namespace Engine::Graphics::D3D12
{
	class DescriptorHeapManager;
}

namespace Engine::Resource
{
	//==========================================================
	// ラスタライザパイプライン用
	//==========================================================
	struct RasterizationMesh
	{
		// 作成
		void Create(
			Graphics::D3D12::Device* a_pDevice,
			Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face,
			DXGI_FORMAT a_indexFormat
		);
		// 解放
		void Release();

		Graphics::D3D12::DynamicVertexBuffer<MeshVertexFloat> vertexBuffer;		// 頂点バッファ
		Graphics::D3D12::DynamicIndexBuffer					indexBuffer;		// インデックスバッファ
	};
}