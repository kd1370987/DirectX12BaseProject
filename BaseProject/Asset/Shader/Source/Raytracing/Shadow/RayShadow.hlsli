// ルートパラメーターの構造体
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/LightData.hlsli"
#include "../../../Common/RootParameters/RaytracingData.hlsli"

// ヘルパー関数
#include "../Raytracing.hlsli"
#include "../../../Common/Math/CalcNormal.hlsli"

//==========================================================================================
// グローバルルートパラメーター
//
//   CBV(b0)         カメラ
//   CBV(b1)         GBufferのSRVインデックス
//   CBV(b10)        主光源(平行光の向きと色)
//   CBV(b2)         フォグ用の影の設定(出力の番号・歩く範囲と歩数)
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

cbuffer CBSunLight : register(b10)
{
	SunLightData g_sun;
}

// フォグ用の影(視線に沿った日なたの割合)の設定。
// ※ CPU 側(RaytracingShadowPass の VolumeShadowParam)と並びを合わせること
struct VolumeShadowParam
{
	uint outIndex;		// 出力(RWTexture2D<float2>)の UAV の番号。VOLUME_SHADOW_NONE なら作らない
	float distance;		// 歩く範囲(ビュー空間の奥行き。シーンの影の距離)
	uint frame;			// 歩く位置のずらしをフレームごとに変える
	uint stepCount;		// 視線を歩く歩数(1歩ごとにレイを1本)
};

// 出力が無いときの番号(パス側と合わせる)
#define VOLUME_SHADOW_NONE 0xFFFFFFFF

cbuffer CBVolumeShadow : register(b2)
{
	VolumeShadowParam g_volume;
}

RaytracingAccelerationStructure	g_raytracingWorld	: register(t0);

RWTexture2D<float4>	gOutPut	: register(u0);
sampler				gSamp	: register(s0);
