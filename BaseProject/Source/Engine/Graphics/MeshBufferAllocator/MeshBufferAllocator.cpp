#include "MeshBufferAllocator.h"

namespace Engine::Graphics
{

	void MeshBufferAllocator::Init(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList,
		uint32_t a_maxVert,
		uint32_t a_maxMeshlets,
		uint32_t a_maxUniqueVertexIndices,
		uint32_t a_maxPrimitiveIndices
	)
	{
		// 頂点用クラス準備
		m_vertexBuffer.Create(a_pDevice,a_pCmdList,a_maxVert,sizeof(Resource::MeshVertexFloat));
		m_vertexAllocator.Init(a_maxVert);

		// メッシュレット用クラス準備
		m_meshletBuffer.Create(a_pDevice, a_pCmdList, a_maxMeshlets, nullptr);
		m_meshletAllocator.Init(a_maxMeshlets);

		// ユニーク頂点インデックスバッファ
		m_uniqueVertexIndices.Create(a_pDevice, a_pCmdList, a_maxUniqueVertexIndices, sizeof(uint8_t));
		m_uniqueVertexIndexAllocator.Init(a_maxUniqueVertexIndices);

		// プリミティブインデックスバッファ
		m_primitiveIndices.Create(a_pDevice, a_pCmdList, a_maxPrimitiveIndices, sizeof(uint32_t));
		m_primitiveIndexAllocator.Init(a_maxPrimitiveIndices);
	}

	void MeshBufferAllocator::Release()
	{
		// D3D12リソースの解放（ラッパークラスの仕様に合わせて変更してください）
		m_vertexBuffer.Release();
		m_meshletBuffer.Release();
		m_uniqueVertexIndices.Release();
		m_primitiveIndices.Release();

		// アロケータの初期化（0クリア）
		m_vertexAllocator.Init(0);
		m_meshletAllocator.Init(0);
		m_uniqueVertexIndexAllocator.Init(0);
		m_primitiveIndexAllocator.Init(0);
	}

	void MeshBufferAllocator::Clear()
	{
		m_vertexAllocator.Init(static_cast<uint32_t>(m_vertexBuffer.GetElementNum()));
		m_meshletAllocator.Init(static_cast<uint32_t>(m_meshletBuffer.GetElementNum()));
		m_uniqueVertexIndexAllocator.Init(static_cast<uint32_t>(m_uniqueVertexIndices.GetElementNum()));
		m_primitiveIndexAllocator.Init(static_cast<uint32_t>(m_primitiveIndices.GetElementNum()));
	}

	MeshAllocationHandle MeshBufferAllocator::AllocateAndUpload(
		D3D12::GraphicsCommandList* a_pCmdList,
		const Resource::Mesh& a_newMeshData
	)
	{
		MeshAllocationHandle _handle = {};
		auto& _meshData = a_newMeshData.GetMeshShaderData();
		auto& _meshMetaData = a_newMeshData.GetMetaData();
		auto& _meshRasterData = a_newMeshData.GetRasterData();

		// 各バッファから必要なサイズを確保
		_handle.vertexHandle = m_vertexAllocator.AllocateRange(static_cast<uint32_t>(_meshData.meshlets.size()));
		_handle.meshletHandle = m_meshletAllocator.AllocateRange(_meshData.meshlets.size());
		_handle.uniqueVertexHandle = m_uniqueVertexIndexAllocator.AllocateRange(_meshData.uniqueVertexIndices.size());
		_handle.primitiveHandle = m_primitiveIndexAllocator.AllocateRange(_meshData.primitiveIndices.size());

		// 確保失敗チェック（どれか一つでも失敗したら全ロールバック）
		if (!_handle.vertexHandle.isValid() || !_handle.meshletHandle.isValid() ||
			!_handle.uniqueVertexHandle.isValid() || !_handle.primitiveHandle.isValid())
		{
			// 失敗した場合は、フェンス待ちなし（0）で即時解放して無効なハンドルを返す
			Free(_handle, 0);
			return MeshAllocationHandle(); // 無効なハンドル
		}

		// UploadHeapからDefaultHeapへデータをコピーするコマンドを積む
		 m_vertexBuffer.UploadDataRange(
			 a_pCmdList, 
			 _handle.vertexHandle.startIndex,
			 _handle.vertexHandle.count,
			 (void*)a_newMeshData.GetVertexVec().data()
		 );
		 m_meshletBuffer.UploadDataRange(
			 a_pCmdList, 
			 _handle.meshletHandle.startIndex,
			 _handle.meshletHandle.count,
			 _meshData.meshlets.data()
		 );
		m_uniqueVertexIndices.UploadDataRange(
			a_pCmdList, 
			_handle.uniqueVertexHandle.startIndex,
			_handle.uniqueVertexHandle.count,
			_meshData.uniqueVertexIndices.data()
		);
		m_primitiveIndices.UploadDataRange(
			a_pCmdList,
			_handle.primitiveHandle.startIndex,
			_handle.primitiveHandle.count,
			_meshData.primitiveIndices.data()
		);

		return _handle;
	}

	void MeshBufferAllocator::Free(const MeshAllocationHandle& a_handle, uint64_t a_currentFenceValue)
	{
		// 各アロケータに遅延解放を依頼する
		if (a_handle.vertexHandle.isValid())
			m_vertexAllocator.FreeRange(a_handle.vertexHandle, a_currentFenceValue);

		if (a_handle.meshletHandle.isValid())
			m_meshletAllocator.FreeRange(a_handle.meshletHandle, a_currentFenceValue);

		if (a_handle.uniqueVertexHandle.isValid())
			m_uniqueVertexIndexAllocator.FreeRange(a_handle.uniqueVertexHandle, a_currentFenceValue);

		if (a_handle.primitiveHandle.isValid())
			m_primitiveIndexAllocator.FreeRange(a_handle.primitiveHandle, a_currentFenceValue);
	}

	void MeshBufferAllocator::BindBuffers(D3D12::GraphicsCommandList* a_pCmdList)
	{
		// 巨大バッファを一括でバインド
		 a_pCmdList->SetGraphicsRootShaderResourceView(2, m_meshletBuffer.GetGPUVirtualAddress());
		 a_pCmdList->SetGraphicsRootShaderResourceView(3, m_uniqueVertexIndices.GetGPUVirtualAddress());
		 a_pCmdList->SetGraphicsRootShaderResourceView(4, m_primitiveIndices.GetGPUVirtualAddress());
		 a_pCmdList->SetGraphicsRootShaderResourceView(5, m_vertexBuffer.GetGPUVirtualAddress());
	}

	void MeshBufferAllocator::UpdateFrame(D3D12::GraphicsCommandList* a_pCmdList,uint64_t a_completedFenceValue)
	{
		// 毎フレーム呼び出し、GPUが使い終わった領域をフリーリストに戻す
		m_vertexAllocator.UpdateFrees(a_completedFenceValue);
		m_meshletAllocator.UpdateFrees(a_completedFenceValue);
		m_uniqueVertexIndexAllocator.UpdateFrees(a_completedFenceValue);
		m_primitiveIndexAllocator.UpdateFrees(a_completedFenceValue);

		// GPUデータの更新
		m_vertexBuffer.Update(a_pCmdList);
		m_meshletBuffer.Update(a_pCmdList);
		m_uniqueVertexIndices.Update(a_pCmdList);
		m_primitiveIndices.Update(a_pCmdList);
	}
}
