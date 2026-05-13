#pragma once

namespace Engine::Raytracing
{
	class BLAS
	{
	public:
		// 頂点バッファとインデックスバッファから作成
		void Create(
			const D3D12::VertexBuffer<Resource::RTVertex>& a_vertexBuffer,
			const IndexBuffer& a_indexBuffer
		);
		// ジオメトリ情報から作成
		void Create(const D3D12_RAYTRACING_GEOMETRY_DESC& a_desc);
		void Create(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_desc);

		bool Build(
			ID3D12Device5* a_pDevice,
			ID3D12GraphicsCommandList4* a_cmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec
		);

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
		{
			return m_cpResource->GetGPUVirtualAddress();
		}

	private:

		// BLAS本体
		ComPtr<ID3D12Resource> m_cpResource = nullptr;

		// scratchBuffer
		ComPtr<ID3D12Resource> m_cpScratch = nullptr;

		// ジオメトリー
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescVec = {};
	};
}