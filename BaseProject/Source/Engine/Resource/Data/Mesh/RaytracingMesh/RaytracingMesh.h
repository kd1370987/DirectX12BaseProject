#pragma once
namespace Engine::Resource
{
	//==========================================================
	// レイトレ用データ
	//==========================================================
	struct RaytracingMesh
	{
		/// <summary>
		/// BLASの構築コマンドをコンテキストのコンピュートリストへ積む
		/// 参照するメガバッファへの転送はコピーリスト側で積まれるため、
		/// 実行時にキュー間の待ちを張る必要がある(D3D12Wrapper::EndAsyncBuildBatchが担当)
		/// </summary>
		void Create(
			const ResourceBuildContext& a_ctx,
			const std::vector<MeshSubset>& a_subset
		);

		// 解放
		void Release();

		Engine::Raytracing::BLAS blas;
		RangeHandle<MeshVertexFloat> vertexHandle = {};
		RangeHandle<uint32_t> indexHandle = {};
	};
}