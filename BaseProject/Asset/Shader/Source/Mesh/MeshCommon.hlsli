// ---- ルートパラメーター ----
#include "../../Common/CB/CBCamera.hlsli"

// インスタンスごとのデータ : サブメッシュ単位なため、参照するマテリアルは一つ
struct InstanceData
{
	float4x4 worldMat;			// 現在フレームのワールド行列
	float4x4 prevWorldMat;		// １フレーム前のワールド行列

	// メッシュが参照するマテリアル
	uint materialOffset;

	// メガバッファから自分のメッシュデータを見つけるためのオフセット
	uint meshletOffset;			// m_meshletBuffer の開始インデックス
	uint vertexOffset;			// m_vertexBuffer 内のベース頂点インデックス（またはバイトオフセット）
	uint uviOffset;
	uint primitiveOffset;		// m_primitiveIndices 内のベースインデックス
	
	// アニメーションがあればデータが入っている
	uint boneStartIndex;			// 開始位置
	uint boneCount;					// 配列サイズ
};

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
// メッシュレット構造体
struct Meshlet
{
	uint vertexCount;		// このmeshレットが持つ頂点数（最大64）
	uint vertexOffset;		// 頂点インデックス配列におけるスタート位置
	uint primitiveCount;	// このmeshレットが持つポリゴン数（最大126）
	uint primitiveOffset;	// プリミティブ配列におけるスタート位置
};
// 頂点情報
struct Vertex
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // uv座標
	float3 tangent : TANGENT; // 接空間
	float4 color : COLOR; // 頂点色
	uint2 skinIndex : SKININDEX; // スキンメッシュのボーンインデックス（何番目のボーンに影響しているかのデータ（最大４））
	float4 skinWeight : SKINWEIGHT; // ボーンの影響度（最大４）
};
// アニメーション用バッファ
struct BonePallet
{
	row_major float4x4 mat;
};

StructuredBuffer<InstanceData> g_instanceData : register(t0);
StructuredBuffer<Material> g_materialData : register(t1);

StructuredBuffer<Meshlet> g_meshletData : register(t2);
StructuredBuffer<uint> g_uniqueVertexIndices : register(t3); // 各meshレットが使う頂点番号リスト
StructuredBuffer<uint> g_primitiveIndices : register(t4);

StructuredBuffer<Vertex> g_vertices : register(t5);
StructuredBuffer<BonePallet> g_bonePalletData : register(t6);
cbuffer RootConstants : register(b1)
{
	uint g_baseInstanceIndex; // C++から渡されるインスタンス配列のオフセット
};

SamplerState smp : register(s0);

// サンプラー

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
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
    "CBV(b0)," \
    "SRV(t0)," \
    "SRV(t1)," \
    "SRV(t2)," \
    "SRV(t3)," \
    "SRV(t4)," \
    "SRV(t5)," \
    "SRV(t6)," \
    "RootConstants(num32BitConstants=1, b1)," \
    "StaticSampler(s0, " \
    "    filter = FILTER_MIN_MAG_MIP_LINEAR, " \
    "    addressU = TEXTURE_ADDRESS_WRAP, " \
    "    addressV = TEXTURE_ADDRESS_WRAP, " \
    "    addressW = TEXTURE_ADDRESS_WRAP)"


// ==========================================================
// G-Bufferパスのピクセルシェーダーへ渡す出力構造体
// ==========================================================
struct VertexOutput
{
	float4 pos : SV_Position;
	float3 worldPos : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
    
    // モーションベクター（Velocity）用
	float4 curClipPos : POSITION1;
	float4 prevClipPos : POSITION2;

	uint instanceID : INSTANCE_ID;
};
