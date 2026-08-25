// レイトレースパスのバッファ要素。
// BLAS単位で切り出すため、ラスタライザパスとはオフセットの持ち方が違う
#ifndef ROOTPARAM_RAYTRACING_DATA_HLSLI
#define ROOTPARAM_RAYTRACING_DATA_HLSLI

struct RayInstanceData
{
	uint materialOffset;		// このメッシュのマテリアル群の開始位置
	uint vertexStart;
	uint indexStart;
	uint indexCount;

	// アニメーションするインスタンスは、当たり判定(BLAS)だけでなく
	// ヒットシェーダの座標・法線・接線もスキニング済み頂点から取る
	// (バインドポーズの静的頂点と食い違うと影や反射がずれる)
	uint isAnimated;
	uint animatedVertexStart;
	uint2 pad;
};

struct RayMaterial
{
	float4 baseColor;

	float3 emissive;
	float metallic;

	float roughness;
	int baseIndex;
	int metaRoughnessIndex;
	int emissiveIndex;

	int normalIndex;
	uint startIndexLocation;	// このサブメッシュのインデックスバッファ開始位置
	float2 pad;

	// マテリアルから独立した自己発光(加算)。
	// emissive はマテリアルの発光色に掛ける倍率なので、
	// 発光しないマテリアル(emissive = 0)は何倍しても光らない
	float3 emissiveAdd;
	float pad1;
};

// レイ生成シェーダーがGBufferを引くためのSRVインデックス。
// GBufferはディスクリプタヒープ直接参照で読むのでテーブルを張らない
struct RayShadowGBufferIndex
{
	int depth;
	int normal;
	float2 pad;
};

struct RayGIGBufferIndex
{
	int depth;
	int normal;
	int frameCount;		// フレームごとに変える乱数の種
};

#endif
