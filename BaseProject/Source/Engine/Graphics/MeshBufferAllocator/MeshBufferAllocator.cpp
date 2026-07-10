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
	}

	void MeshBufferAllocator::Release()
	{
		m_staticVerticesBuffer.Release();
		m_indexBuffer.Release();
		m_animatedVertexBuffer.Release();
	}

	void MeshBufferAllocator::UpdateFrame(D3D12::GraphicsCommandList* a_pCmdList,uint64_t a_completedFenceValue)
	{
		// メッシュ用のバッファ更新
		m_staticVerticesBuffer.Update(a_completedFenceValue);
		m_indexBuffer.Update(a_completedFenceValue);
		m_animatedVertexBuffer.Update(a_completedFenceValue);
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
}
