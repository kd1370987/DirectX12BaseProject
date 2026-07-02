// ---- ルートパラメーター ----
#include "../../Common/CB/CBCamera.hlsli"

// インスタンスごとのデータ : サブメッシュ単位なため、参照するマテリアルは一つ
struct InstanceData
{
	float4x4 worldMat;			// 現在フレームのワールド行列
	float4x4 prevWorldMat;		// １フレーム前のワールド行列

	// メッシュが参照するマテリアル
	uint materialOffset;
	
	// アニメーションがあればデータが入っている
	uint boneStartIndex;			// 開始位置
	uint boneCount;					// 配列サイズ
};
StructuredBuffer<InstanceData> g_instanceData : register(t0);

// マテリアルの情報
struct Material
{
	// マテリアルのテクスチャスケール値
	float4 baseColor;
	
	float3 emissive;
	float metallic;
	
	float roughness;

	float3 pad;

	// テクスチャのSRVインデックス
	int albedIndex;					// アルベド
	int metaRoughnessIndex;			// メタリックラフネステクスチャ
	int emissiveIndex;
	int normalIndex;
};
StructuredBuffer<Material> g_materialData : register(t1);

// メッシュレット構造体
struct Meshlet
{
	uint vertexCount;		// このmeshレットが持つ頂点数（最大64）
	uint vertexOffset;		// 頂点インデックス配列におけるスタート位置
	uint primitiveCount;	// このmeshレットが持つポリゴン数（最大126）
	uint primitiveOffset;	// プリミティブ配列におけるスタート位置
};
StructuredBuffer<Meshlet> g_meshletData : register(t2);
ByteAddressBuffer g_uniqueVertexIndices : register(t3); // 各meshレットが使う頂点番号リスト
ByteAddressBuffer g_primitiveIndices : register(t4);

// 頂点情報
ByteAddressBuffer g_vertices : register(t5);

// アニメーション用バッファ
struct BonePallet
{
	row_major float4x4 mat;
};
StructuredBuffer<BonePallet> g_bonePalletData : register(t6);


// ---- ルートシグネチャ設定 : 全メッシュシェーダーで共通して使う ----
// カメラ定数バッファ
// インスタンスごとのデータ
// マテリアルごとのデータ
// メッシュレット
// ユニーク頂点インデックス
// プリミティブインデックス
// 頂点配列
// アニメーション用ボーン配列
#define MESHGLOBAL_ROOT_SIG \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
    "CBV(b0)," \
    "SRV(t0)," \
    "SRV(t1)," \
    "SRV(t2)," \
    "SRV(t3)," \
    "SRV(t4)," \
    "SRV(t5)," \
    "SRV(t6)," \
    "StaticSampler(s0, filter=FILTER_ANISOTROPIC)"
