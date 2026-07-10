#pragma once

#include "MeshAllocationHandle.h"

namespace Engine::Graphics
{
	struct BufferSizeDesc
	{
		size_t staticVertexBufferSize = 0;
		size_t indexBufferSize = 0;
		UINT animatedVertexBufferSize = 0;
	};

	/// <summary>
	/// メッシュシェーダーのためのメガバッファを管理するためのクラス
	/// メッシュシェーダーで描画するものはここからハンドルをもらい受ける必要がある
	/// 静的なオブジェクトのみで、毎フレーム更新されるようなものはレンダーコンテキスト側が保持
	/// </summary>
	class MeshBufferAllocator
	{
	public:
		/// <summary>
		/// ゲーム初期化時にメッシュシェーダー用のメモリ領域を確保する
		/// </summary>
		/// <param name="a_pDevice">デバイスポインタ</param>
		/// <param name="a_maxVert">最大頂点数</param>
		/// <param name="a_maxMeshlets">最大メッシュレット数</param>
		void Init(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			const BufferSizeDesc& a_bufferSizes
		);

		/// <summary>
		/// 解放処理 : ゲーム終了時に呼び出し
		/// </summary>
		void Release();

		
		/// <summary>
		/// 毎フレーム呼ばれる : 遅延開放処理
		/// </summary>
		/// <param name="currentFenceValue">フェンス値</param>
		void UpdateFrame(D3D12::GraphicsCommandList* a_pCmdList,uint64_t a_completedFenceValue);

		//--------------------------------------------------------------------------------------------
		// バッファにアロケート
		//--------------------------------------------------------------------------------------------
		RangeHandle<Resource::MeshVertexFloat> AllocateVertex(const std::vector<Resource::MeshVertexFloat>& a_vertex);		// 静的頂点
		RangeHandle<uint32_t> AllocateIndex(const std::vector<uint32_t>& a_indices);										// インデックス
		RangeHandle<Resource::MeshVertexFloat> AllocateAnimatedVertex(UINT a_size);											// 動的頂点

		//--------------------------------------------------------------------------------------------
		// ハンドルの返却
		//--------------------------------------------------------------------------------------------
		void StaticVertexFree(const RangeHandle<Resource::MeshVertexFloat>& a_handle);
		void IndexFree(const RangeHandle<uint32_t>& a_handle);
		void AnimatedVertexFree(const RangeHandle<Resource::MeshVertexFloat>& a_handle);

		//--------------------------------------------------------------------------------------------
		// バッファアクセス
		//--------------------------------------------------------------------------------------------
		const D3D12::MegaStructuredBuffer<Resource::MeshVertexFloat>& GetStaticVertexBuffer() const { return m_staticVerticesBuffer; }
		const D3D12::MegaStructuredBuffer<uint32_t>& GetIndexBuffer() const { return m_indexBuffer; }
		const D3D12::MegaRWStructuredBuffer<Resource::MeshVertexFloat>& GetAnimatedVertexBuffer() const { return m_animatedVertexBuffer; }

		D3D12::MegaStructuredBuffer<Resource::MeshVertexFloat>& RefStaticVertexBuffer() { return m_staticVerticesBuffer; }
		D3D12::MegaStructuredBuffer<uint32_t>& RefIndexBuffer() { return m_indexBuffer; }
		D3D12::MegaRWStructuredBuffer<Resource::MeshVertexFloat>& RefAnimatedVertexBuffer() { return m_animatedVertexBuffer; }

	private:

		//--------------------------------------------------------------------------------------------
		// メッシュデータ
		//--------------------------------------------------------------------------------------------
		D3D12::MegaStructuredBuffer<Resource::MeshVertexFloat>		m_staticVerticesBuffer;		// 静的な頂点データ
		D3D12::MegaStructuredBuffer<uint32_t>						m_indexBuffer;				// インデックスバッファ
		D3D12::MegaRWStructuredBuffer<Resource::MeshVertexFloat>	m_animatedVertexBuffer;		// スキニング後の頂点バッファ
	};
}