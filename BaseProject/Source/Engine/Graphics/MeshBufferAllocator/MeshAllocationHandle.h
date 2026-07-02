#pragma once
namespace Engine::Graphics
{

	// モデルに渡すハンドル
	struct IndexRangeHandle
	{
		uint32_t startIndex;
		uint32_t count;
		uint32_t allocationId; // 世代管理の代わり
		bool isValid() const { return count > 0; }
	};

	/// <summary>
	/// メッシュシェーダーで描画されるものすべてが持つ
	/// </summary>
	struct MeshAllocationHandle
	{
		IndexRangeHandle vertexHandle;				// 頂点開始位置
		IndexRangeHandle meshletHandle;				// メッシュレット開始位置
		IndexRangeHandle primitiveHandle;			// プリミティブ開始位置
		IndexRangeHandle uniqueVertexHandle;		// 頂点インデックス開始位置
		bool     isValid = false;			// 使用中か否か
	};
}