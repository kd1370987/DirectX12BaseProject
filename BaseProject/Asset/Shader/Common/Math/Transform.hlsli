
#ifndef TRANSFORM_HLSLI
#define TRANSFORM_HLSLI

#include "../CB/CBCamera.hlsli"

// 頂点座標をワールド座標へ
// 行ベクトルと列行列を期待する
float4 Transform_LocalToWorld(float3 a_localPos,float4x4 a_worldMat)
{
	return mul(float4(a_localPos, 1), a_worldMat);
}

// ワールド座標をビュー座標へ
float4 Transform_WorldToView(float4 a_worldPos)
{
	return mul(a_worldPos, g_camera.view);
}

// ビュー座標をプロジェクション座標へ
float4 Transform_ViewToProj(float4 a_viewPos)
{
	return mul(a_viewPos,g_camera.proj);
}

// 頂点からプロジェクションまで一発
float4 Transform_LocalToProj(float3 a_localPos, float4x4 a_worldMat)
{
	float4 _worldPos = Transform_LocalToWorld(a_localPos, a_worldMat);
	float4 _viewPos = Transform_WorldToView(_worldPos);
	return Transform_ViewToProj(_viewPos);
}

#endif
