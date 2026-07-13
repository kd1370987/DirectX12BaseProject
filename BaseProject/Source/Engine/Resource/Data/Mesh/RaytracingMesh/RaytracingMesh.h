#pragma once
namespace Engine::Resource
{
	//==========================================================
	// レイトレ用データ
	//==========================================================
	struct RaytracingMesh
	{
		void Create(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<MeshSubset>& a_subset
		);

		// 解放
		void Release();

		Engine::Raytracing::BLAS blas;
		RangeHandle<MeshVertexFloat> vertexHandle = {};
		RangeHandle<uint32_t> indexHandle = {};
	};
}