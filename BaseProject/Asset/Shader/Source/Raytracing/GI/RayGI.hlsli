// ルートパラメーターの構造体
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/AmbientData.hlsli"
#include "../../../Common/RootParameters/RaytracingData.hlsli"
#include "../../../Common/RootParameters/Vertex.hlsli"

// ヘルパー関数
#include "../Raytracing.hlsli"
#include "../../../Common/Math/CalcNormal.hlsli"

//==========================================================================================
// グローバルルートパラメーター
//
//   0 : CBV(b0)         カメラ
//   1 : SRV(t0)         TLAS
//   2 : UAVテーブル(u0) GI出力(ハーフ解像度)
//   3 : SRVテーブル(t1) インスタンス
//   4 : SRVテーブル(t2) マテリアル
//   5 : CBV(b1)         GBufferのSRVインデックス
//   6 : CBV(b10)        環境光
//   7 : SRVテーブル(t3) 頂点
//   8 : SRVテーブル(t4) インデックス
//   9 : SRVテーブル(t5) スキニング済み頂点
//
// 実体は C++ 側(RaytracingGIPass)で組む。並びを変えるときは両方を揃えること
//==========================================================================================
cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBGBufferIndex : register(b1)
{
	RayGIGBufferIndex g_gbuffer;
}

cbuffer CBAmbient : register(b10)
{
	AmbientData g_ambient;
}

RaytracingAccelerationStructure	g_raytracingWorld	: register(t0);
StructuredBuffer<RayInstanceData>	g_instanceData		: register(t1);
StructuredBuffer<RayMaterial>		g_materialData		: register(t2);
StructuredBuffer<Vertex>			g_vertexfloatData	: register(t3);
StructuredBuffer<uint>				g_indexData			: register(t4);
StructuredBuffer<Vertex>			g_animatedVertexData: register(t5);

RWTexture2D<float4>	gOutPut	: register(u0);
sampler				gSamp	: register(s0);
