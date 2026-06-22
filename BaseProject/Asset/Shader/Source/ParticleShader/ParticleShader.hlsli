// ヘルパー関数
#include "../../Common/Math/Transform.hlsli"
#include "../../Common/Math/Normal.hlsli"

// ルートパラメター
#include "../../Common/CB/CBCamera.hlsli"
#include "../../Common/RootParameters/Particle.hlsli"

#include "../RootSignatureLayout.hlsli"

// ルートシグネチャ
#define PARTICLE_ROOT_SIG \
RS_FLAGS","\
RS_CAMERA_CB","\
"DescriptorTable(SRV(t0, numDescriptors=1),visibility = SHADER_VISIBILITY_VERTEX), " \
"DescriptorTable(SRV(t1, numDescriptors=1),visibility = SHADER_VISIBILITY_PIXEL), " \
RS_STATIC_SAMPLER

// パーティクルデータ
StructuredBuffer<ParticleData> g_particleBuffer : register(t0);
Texture2D g_mainTex : register(t1);

// サンプラー
SamplerState g_samp : register(s0);

// 頂点入力
struct VSInput
{
	float4 pos	: POSITION;			// 頂点座標
	float2 uv	: TEXCOORD0;		// UV座標
	uint instID : SV_InstanceID;	// インスタンス番号
};

// 頂点出力
struct VSOutput
{
	float4 pos : SV_Position;		// 射影行列
	
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
};

