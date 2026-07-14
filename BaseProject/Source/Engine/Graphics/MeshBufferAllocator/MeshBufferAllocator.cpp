#include "MeshBufferAllocator.h"

namespace Engine::Graphics
{

	void MeshBufferAllocator::Init(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList,
		const BufferSizeDesc& a_bufferSizes
	)
	{
		// メッシュ用バッファ作成 : サイズはもらい受ける
		m_staticVerticesBuffer.Create(a_pDevice,a_pCmdList,a_bufferSizes.staticVertexBufferSize);
		m_indexBuffer.Create(a_pDevice,a_pCmdList,a_bufferSizes.indexBufferSize);
		m_animatedVertexBuffer.Create(a_pDevice,a_bufferSizes.animatedVertexBufferSize);

		size_t _size = sizeof(MeshInstanceData);

		m_meshletBuffer.Create(a_pDevice,a_pCmdList,10000);
		m_uniqueVertexIndicesBuffer.Create(a_pDevice, a_pCmdList, 1000000);
		m_meshTriangleBuffer.Create(a_pDevice, a_pCmdList, 1000000);
		m_meshletCullDataBuffer.Create(a_pDevice,a_pCmdList,1000000);
	}

	void MeshBufferAllocator::Release()
	{
		m_staticVerticesBuffer.Release();
		m_indexBuffer.Release();
		m_animatedVertexBuffer.Release();

		m_meshletBuffer.Release();
		m_uniqueVertexIndicesBuffer.Release();
		m_meshTriangleBuffer.Release();
		m_meshletCullDataBuffer.Release();
	}

	void MeshBufferAllocator::UpdateFrame(D3D12::GraphicsCommandList* a_pCmdList,uint64_t a_completedFenceValue)
	{
		// メッシュ用のバッファ更新
		m_staticVerticesBuffer.Update(a_completedFenceValue);
		m_indexBuffer.Update(a_completedFenceValue);
		m_animatedVertexBuffer.Update(a_completedFenceValue);

		m_meshletBuffer.Update(a_completedFenceValue);
		m_uniqueVertexIndicesBuffer.Update(a_completedFenceValue);
		m_meshTriangleBuffer.Update(a_completedFenceValue);
		m_meshletCullDataBuffer.Update(a_completedFenceValue);
	}
	RangeHandle<Resource::MeshVertexFloat> MeshBufferAllocator::AllocateVertex(const std::vector<Resource::MeshVertexFloat>& a_vertex)
	{
		return m_staticVerticesBuffer.AllocateAndUpload(a_vertex.data(), static_cast<UINT>(a_vertex.size()));
	}
	RangeHandle<uint32_t> MeshBufferAllocator::AllocateIndex(const std::vector<uint32_t>& a_indices)
	{
		return m_indexBuffer.AllocateAndUpload(a_indices.data(), static_cast<UINT>(a_indices.size()));
	}
	RangeHandle<Resource::MeshVertexFloat> MeshBufferAllocator::AllocateAnimatedVertex(UINT a_size)
	{
		return m_animatedVertexBuffer.Allocate(a_size);
	}
	RangeHandle<Resource::Meshlet> MeshBufferAllocator::AllocateMeshlet(const std::vector<Resource::Meshlet>& a_meshlets)
	{
		return m_meshletBuffer.AllocateAndUpload(a_meshlets.data(),static_cast<UINT>(a_meshlets.size()));
	}
	RangeHandle<uint32_t> MeshBufferAllocator::AllocateUniqueVertIndices(const std::vector<uint32_t>& a_uniqueVertIndices)
	{
		return m_uniqueVertexIndicesBuffer.AllocateAndUpload(a_uniqueVertIndices.data(), static_cast<UINT>(a_uniqueVertIndices.size()));
	}
	RangeHandle<DirectX::MeshletTriangle> MeshBufferAllocator::AllocateTriangles(const std::vector<DirectX::MeshletTriangle>& a_triangles)
	{
		return m_meshTriangleBuffer.AllocateAndUpload(a_triangles.data(), static_cast<UINT>(a_triangles.size()));
	}
	RangeHandle<DirectX::CullData> MeshBufferAllocator::AllocateCullData(const std::vector<DirectX::CullData>& a_cullData)
	{
		return m_meshletCullDataBuffer.AllocateAndUpload(a_cullData.data(),static_cast<UINT>(a_cullData.size()));
	}
	void MeshBufferAllocator::StaticVertexFree(const RangeHandle<Resource::MeshVertexFloat>& a_handle)
	{
		ENGINE_LOG("静的頂点データバッファ : Free");
		m_staticVerticesBuffer.Free(a_handle);
	}
	void MeshBufferAllocator::IndexFree(const RangeHandle<uint32_t>&a_handle)
	{
		ENGINE_LOG("インデックスデータバッファ : Free");
		m_indexBuffer.Free(a_handle);
	}
	void MeshBufferAllocator::AnimatedVertexFree(const RangeHandle<Resource::MeshVertexFloat>&a_handle)
	{
		ENGINE_LOG("アニメーション後頂点データバッファ : Free");
		m_animatedVertexBuffer.Free(a_handle);
	}
	void MeshBufferAllocator::MeshletFree(const RangeHandle<Resource::Meshlet>& a_handle)
	{
		ENGINE_LOG("メッシュレット : Free");
		m_meshletBuffer.Free(a_handle);
	}
	void MeshBufferAllocator::UniqueVertIndicesFree(const RangeHandle<uint32_t>&a_handle)
	{
		ENGINE_LOG("ユニーク頂点インデックス : Free");
		m_uniqueVertexIndicesBuffer.Free(a_handle);
	}
	void MeshBufferAllocator::TrianglesFree(const RangeHandle<DirectX::MeshletTriangle>&a_handle)
	{
		ENGINE_LOG("メッシュトライアングル : Free");
		m_meshTriangleBuffer.Free(a_handle);
	}
	void MeshBufferAllocator::MeshletCullDataFree(const RangeHandle<DirectX::CullData>& a_handle)
	{
		ENGINE_LOG("メッシュレット当たり判定データ : Free");
		m_meshletCullDataBuffer.Free(a_handle);
	}
}
