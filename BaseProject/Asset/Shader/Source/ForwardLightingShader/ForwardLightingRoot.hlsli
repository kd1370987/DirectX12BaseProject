#include "../RootSignatureLayout.hlsli"

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

#define FOWARDLIGHTING_ROOT_SIG \
RS_FLAGS ","\
RS_CAMERA_CB ","\
RS_BUFFERINDEX_CB ","\
RS_INSTANCE_DATA_TABLE ","\
RS_SUBSET_DATA_TABLE ","\
RS_BONEPALLET_DATA_TABLE ","\
RS_MATERIAL_TABLE ","\
"CBV(b5,visibility = SHADER_VISIBILITY_ALL),"\
RS_STATIC_SAMPLER

cbuffer cbAmbient : register(b5)
{
	float4 ambientLightColor;

	float4 DL_dir;
	float4 DL_color;
}

// サンプラー
SamplerState g_samp : register(s0);

// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_POSITION; // 変換された座標
	float4 color : COLOR; // 変換された色
	float2 uv : TEXCOORD; // uv座標
	float3 normal : NORMAL; // 法線
	
	float3 wN : TEXCOORD1; // ワールド法線
	float3 wT : TEXCOORD2; // ワールド接線
	float3 wB : TEXCOORD3; // ワールド副接線(従法線)
};
