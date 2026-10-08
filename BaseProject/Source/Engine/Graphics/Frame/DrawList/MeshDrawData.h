#pragma once
namespace Engine::Graphics
{
	// ---- メッシュシェーダー用構造体 ----
	struct MeshInstanceData
	{
		Math::Matrix worldMat;			// 現在フレームのワールド行列

		Math::Matrix prevWorldMat;		// １フレーム前のワールド行列

		uint32_t materialOffset;			// メッシュが参照するマテリアル
		uint32_t meshletOffset;				// メッシュレットオフセット
		uint32_t vertexOffset;				// 頂点オフセット
		uint32_t uviOffset;					// ユニーク頂点インデックスオフセット

		uint32_t primitiveOffset;			// プリミティブオフセット
		uint32_t animatedVertexStart;		// アニメーション頂点オフセット
		uint32_t isAnimated;				// アニメーションするかどうか
		uint32_t cullStart;					// カリングバッファオフセット

		uint32_t meshletCount;				// メッシュレットカウント
		Math::Vector3 pad;
	};
	struct MeshMaterial
	{
		// マテリアルのテクスチャスケール値
		Math::Color baseColor;

		Math::Vector3 emissive;
		float metallic;

		float roughness;

		// マテリアルとは独立した自己発光(ModelComponent の 発光色 × 発光強度)。
		// emissive はエミッシブテクスチャに掛ける倍率なので、テクスチャを持たない
		// モデルは何倍しても光らない。こちらは加算なので単体で光らせられる。
		Math::Vector3 emissiveAdd;

		// テクスチャのSRVインデックス
		int albedoIndex;					// アルベド
		int metaRoughnessIndex;			// メタリックラフネステクスチャ
		int emissiveIndex;
		int normalIndex;
	};
}
