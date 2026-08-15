// インクルードガード
#ifndef SRV_SUBSET_DATA_HLSLI
#define SRV_SUBSET_DATA_HLSLI

struct SubsetData
{
	float4 baseColorScale;
	float3 emissiveColorScale;

	// C++側(Graphics::SubSetData)は float なので int にしてはいけない。
	// サイズは同じで通ってしまうが、floatのビット列をintとして読むことになる。
	float metallic;
	float roughness;

	// マテリアルとは独立した自己発光（ModelComponent の 発光色 × 発光強度）。
	// emissiveColorScale はエミッシブテクスチャに掛ける倍率なので、テクスチャが無い
	// (＝黒テクスチャにフォールバックする)モデルは何倍しても光らない。
	// こちらは加算なので、テクスチャもマテリアルの emissive も要らずに光らせられる。
	// 既定は0で、使わないモデルの見た目は変わらない。
	float3 emissiveAdd;
};

// インスタンスデータ
StructuredBuffer<SubsetData> g_subsetData : register(t1);

#endif

// 共通バッファ
#define RS_SUBSET_DATA_TABLE "DescriptorTable(SRV(t1, numDescriptors=1),visibility = SHADER_VISIBILITY_PIXEL)"
