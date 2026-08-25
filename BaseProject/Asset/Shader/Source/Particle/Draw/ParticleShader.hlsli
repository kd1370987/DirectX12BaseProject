// ルートパラメーターの構造体
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/Particle.hlsli"

#include "../../../Common/RootSignatureLayout.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)         カメラ
//   1 : SRVテーブル(t0) 粒バッファ(更新パスの出力をそのまま読む)
//   2 : SRVテーブル(t1) 絵
//   3 : CBV(b1)         アセット単位の描画設定
//==========================================================================================
#define PARTICLE_ROOT_SIG \
RS_FLAGS","\
"CBV(b0, visibility = SHADER_VISIBILITY_ALL),"\
"DescriptorTable(SRV(t0, numDescriptors=1), visibility = SHADER_VISIBILITY_VERTEX),"\
"DescriptorTable(SRV(t1, numDescriptors=1), visibility = SHADER_VISIBILITY_PIXEL),"\
"CBV(b1, visibility = SHADER_VISIBILITY_VERTEX),"\
RS_STATIC_SAMPLER

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBParticleDraw : register(b1)
{
	ParticleDrawData g_draw;
}

StructuredBuffer<ParticleData> g_particleBuffer : register(t0);
Texture2D g_mainTex : register(t1);

SamplerState g_samp : register(s0);

// ヘルパー関数 : 上で宣言した g_camera を使うので、必ずこの位置で読むこと
#include "../../../Common/Math/Transform.hlsli"
#include "../../../Common/Math/Normal.hlsli"

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
