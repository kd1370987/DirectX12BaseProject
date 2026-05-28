
// ヘルパー関数
#include "../../Common/Math/Transform.hlsli"
#include "../../Common/Math/Normal.hlsli"

// ルートパラメターズ
#include "../../Common/CB/CBCamera.hlsli"
#include "../../Common/RootParameters/BufferIndex.hlsli"
#include "../../Common/RootParameters/InstanceData.hlsli"
#include "../../Common/RootParameters/SubsetData.hlsli"
#include "../../Common/RootParameters/BonePalletData.hlsli"
#include "../../Common/RootParameters/Material.hlsli"

#include "../RootSignatureLayout.hlsli"

#define GBUFFER_ROOT_SIG \
RS_FLAGS ","\
RS_CAMERA_CB ","\
RS_BUFFERINDEX_CB ","\
RS_INSTANCE_DATA_TABLE ","\
RS_SUBSET_DATA_TABLE ","\
RS_BONEPALLET_DATA_TABLE ","\
RS_MATERIAL_TABLE ","\
RS_STATIC_SAMPLER



// サンプラー
SamplerState smp : register(s0);




// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
	float4 color : COLOR; // 変換された色
	float2 uv : TEXCOORD; // uv座標
	float3 normal : NORMAL; // 法線
	
	float3 wN : TEXCOORD1; // ワールド法線
	float3 wT : TEXCOORD2; // ワールド接線
	float3 wB : TEXCOORD3; // ワールド副接線(従法線)
};

// GBuffer用出力
struct PSOutput
{
	float4 albedo : SV_Target0;
	float2 normal : SV_Target1;
	float4 material : SV_Target2;
	float4 emissiv : SV_Target3;
	float2 velocity : SV_Target4;
};




