// ルートパラメーターの構造体
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/AmbientData.hlsli"
#include "../../../Common/RootParameters/RaytracingData.hlsli"

// ヘルパー関数
#include "../Raytracing.hlsli"
#include "../../../Common/Math/CalcNormal.hlsli"

//==========================================================================================
// グローバルルートパラメーター
//
//   CBV(b0)         カメラ
//   CBV(b1)         GBufferのSRVインデックス
//   CBV(b10)        環境光(ライトの向きを取る)
//   SRV(t0)         TLAS
//   UAVテーブル(u0) 影マスク出力
//
// 実体は C++ 側(RaytracingShadowPass)で組む。並びを変えるときは両方を揃えること
//==========================================================================================
cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBGBufferIndex : register(b1)
{
	RayShadowGBufferIndex g_gbuffer;
}

cbuffer CBAmbient : register(b10)
{
	AmbientData g_ambient;
}

RaytracingAccelerationStructure	g_raytracingWorld	: register(t0);

RWTexture2D<float4>	gOutPut	: register(u0);
sampler				gSamp	: register(s0);
