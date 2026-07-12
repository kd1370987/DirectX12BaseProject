#pragma once
namespace Engine::Resource
{
	/// <summary>
	/// サブセットとメッシュレットの紐付けデータ
	/// </summary>
	struct SubsetMeshletData
	{
		uint32_t meshletOffset = 0; // このサブセットの描画を開始するメッシュレットのオフセット
		uint32_t meshletCount = 0;  // このサブセットが持つメッシュレットの総数
	};

	/// <summary>
	/// HLSL側にそのまま転送する用のmeshレットデータ
	/// </summary>
	struct Meshlet
	{
		uint32_t vertexCount = 0;			// このmeshレットが持つ頂点数（最大64）
		uint32_t vertexOffset = 0;			// 頂点インデックス配列におけるスタート位置
		uint32_t primitiveCount = 0;		// このmeshレットが持つポリゴン数（最大126）
		uint32_t primitiveOffset = 0;		// プリミティブ配列におけるスタート位置
	};


	/// <summary>
	/// メッシュシェーダー用データ
	/// </summary>
	struct MeshShaderData
	{
		std::vector<Meshlet> meshlets;								// メッシュレットのカタログ
		std::vector<uint32_t> uniqueVertexIndices;						// 各メッシュレットが使う頂点番号リスト
		std::vector<DirectX::MeshletTriangle> primitiveIndices;		// ローカルなインデックス（0 から 63の範囲）

		// サブセットごとのメッシュレット情報
		std::vector<SubsetMeshletData> subsetMeshlets;

		// 各種データのGPUデータハンドル
		RangeHandle<Resource::Meshlet> meshletHandle;
		RangeHandle<uint32_t> uinqueVertexIndecsHandle;
		RangeHandle<DirectX::MeshletTriangle> primitiveIndicesHandle;
	};
}