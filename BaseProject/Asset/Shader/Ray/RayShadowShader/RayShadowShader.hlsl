#include "../Raytracing.hlsli"
#include "../../Source/CalcNormal.hlsli"

struct GBufferIndex
{
	int depth;
	int normal;
	float2 pad2;
};
cbuffer cbGBufferIndex : register(b1)
{
	GBufferIndex g_gbuffer;
}
struct DL
{
	float3 dir;
	float pad;
};
cbuffer cbLight : register(b2)
{
	DL g_dl;
}
//Texture2D g_depthTex : register(t1);


struct RayPayload
{
	float3 color;
	int hit;
	int depth;
};

float3 ReconstructViewPos(float2 uv, float depth)
{
	float2 ndcXY = uv * 2.0f - 1.0f;
	ndcXY.y = -ndcXY.y; // ★ここでY軸を反転させる！
	
	//float4 clip = float4(uv * 2 - 1, depth, 1);
	float4 clip = float4(ndcXY, depth, 1);
	float4 view = mul(g_camera.invProj, clip);
	return view.xyz / view.w;
}

// レイ生成シェーダー
[shader("raygeneration")]
void RayGen()
{
	uint2 _id = DispatchRaysIndex().xy;
	uint2 _dim = DispatchRaysDimensions().xy;
	
	float2 _uv = (_id + 0.5) / _dim;
	

	// GBuffer取得
	Texture2D<float> _depthTex = ResourceDescriptorHeap[g_gbuffer.depth];
	Texture2D _normalTex = ResourceDescriptorHeap[g_gbuffer.normal];

	// 深度値を取得
	float _depth = _depthTex.Load(int3(_id, 0)).r;
	if(_depth >= 1.0f)
	{
		gOutPut[_id] = float4(1,1,1,1);
		return;
	}
	
	// 法線を取得
	float2 _enc = _normalTex.Load(int3(_id, 0)).rg; // 法線
	float3 _normal = DecsodeNormal(_enc);			// 法線を復元

	// 3D空間での位置を復元
	float3 _viewPos = ReconstructViewPos(_uv, _depth);
	float4 _worldPos4 = mul(float4(_viewPos, 1), g_camera.invView);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;
	
	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	_ray.Origin = _worldPos +_normal * 0.01f; // シャドウアクネ
	//_ray.Direction = normalize(-g_dl.dir);
	_ray.Direction = normalize(float3(0.5, 0.5, 0.2));
	_ray.TMin = 0.001;
	_ray.TMax = 10000;


	RayPayload _payload;
	_payload.color = float3(0, 0, 0);
	_payload.depth = _payload.depth;
	_payload.hit = 0;
	
	TraceRay(
		g_raytracingWorld,
		//RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,// | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0,
		0xFF,
		0,
		0,
		0,
		_ray,
		_payload
	);

	
	//gOutPut[_id] = _payload.hit ? float4(0, 0, 0, 1) : float4(1, 1, 1, 1);
	gOutPut[_id] = float4(_payload.color,1);
}

[shader("closesthit")]
void ShadowCHS(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.color = float3(0,0,0);
	a_payload.hit = 1;
}

[shader("miss")]
void ShadowMiss(inout RayPayload a_payload)
{
	a_payload.color = float3(1,1,1);
	a_payload.hit = 0;
}
