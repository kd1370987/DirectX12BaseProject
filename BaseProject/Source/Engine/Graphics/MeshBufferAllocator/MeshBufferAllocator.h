#pragma once

#include "MeshAllocationHandle.h"

namespace Engine::Graphics
{
	/// <summary>
	/// メッシュシェーダーのためのメガバッファを管理するためのクラス
	/// メッシュシェーダーで描画するものはここからハンドルをもらい受ける必要がある
	/// 静的なオブジェクトのみで、毎フレーム更新されるようなものはレンダーコンテキスト側が保持
	/// </summary>
	class MeshBufferAllocator
	{
	public:
		// ---------------------------------------------------------
		// 初期化と終了
		// ---------------------------------------------------------
		/// <summary>
		/// ゲーム初期化時にメッシュシェーダー用のメモリ領域を確保する
		/// </summary>
		/// <param name="a_pDevice">デバイスポインタ</param>
		/// <param name="a_maxVert">最大頂点数</param>
		/// <param name="a_maxMeshlets">最大メッシュレット数</param>
		void Init(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			uint32_t a_maxVert,
			uint32_t a_maxMeshlets,
			uint32_t a_maxUniqueVertexIndices,
			uint32_t a_maxPrimitiveIndices
		);

		/// <summary>
		/// 解放処理 : ゲーム終了時に呼び出し
		/// </summary>
		void Release();

		/// <summary>
		/// シーンの切り替わりなど、大量に静的データが動かされる場合
		/// メモリの解放はしないが、内容をリセットする
		/// </summary>
		void Clear();

		// ---------------------------------------------------------
		// 貸し出しと回収
		// ---------------------------------------------------------
		/// <summary>
		/// 新しいモデルをロードした時に呼ばれる。空き領域を探してオフセットを返す
		/// ついでにUploadHeapからDefaultHeapへのデータ転送コマンドも積む 
		/// </summary>
		/// <param name="pCmdList">GPU実行のためのコピー用コマンドリスト</param>
		/// <param name="newMeshData">新たに生成されたメッシュ</param>
		/// <returns>割り当てられたハンドルが返る</returns>
		MeshAllocationHandle AllocateAndUpload(
			D3D12::GraphicsCommandList* a_pCmdList,
			const Resource::Mesh& a_newMeshData
		);

		/// <summary>
		/// モデルをアンロードする際に呼ぶ
		/// 上書きするためメモリの解放処理は走らない
		/// </summary>
		/// <param name="a_handle">削除される領域</param>
		/// <param name="a_currentFenceValue">削除される領域</param>
		void Free(const MeshAllocationHandle& a_handle, uint64_t a_currentFenceValue);

		// ---------------------------------------------------------
		// 描画時のサポート
		// ---------------------------------------------------------
		/// <summary>
		/// 描画ループの最初に呼ばれる。巨大バッファをルートシグネチャにセットする 
		/// </summary>
		/// <param name="pCmdList"></param>
		void BindBuffers(D3D12::GraphicsCommandList* a_pCmdList);

		/// <summary>
		/// 毎フレーム呼ばれる : 遅延開放処理
		/// </summary>
		/// <param name="currentFenceValue">フェンス値</param>
		void UpdateFrame(D3D12::GraphicsCommandList* a_pCmdList,uint64_t a_completedFenceValue);

	private:
		// 頂点用バッファと管理クラス
		D3D12::StaticStructuredBuffer<Resource::MeshVertexFloat>	m_vertexBuffer;
		RangeAllocator<Resource::MeshVertexFloat>					m_vertexAllocator;

		// メッシュレットバッファと管理クラス
		D3D12::StaticStructuredBuffer<Resource::Meshlet>			m_meshletBuffer;
		RangeAllocator<Resource::Meshlet>							m_meshletAllocator;

		// ユニーク頂点インデックスバッファと管理クラス
		D3D12::StaticStructuredBuffer<uint32_t>						m_uniqueVertexIndices;
		RangeAllocator<uint32_t>									m_uniqueVertexIndexAllocator;

		// プリミティブインデックスバッファと管理クラス
		D3D12::StaticStructuredBuffer<uint32_t>						m_primitiveIndices;
		RangeAllocator<uint32_t>									m_primitiveIndexAllocator;

		std::mutex m_mutex;
	};
}