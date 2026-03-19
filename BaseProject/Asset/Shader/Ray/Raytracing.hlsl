// カメラの定数バッファ
struct Camera
{
	float4x4 rotMat;	// カメラの回転行列
	float3 pos;			// カメラの座標
	float aspect;		// カメラのアスペクト比
	float farClip;		// 遠平面
	float nearClip;		// 近平面
};

cbuffer cbCamera : register(b0)
{
	Camera g_camera;
}

RaytracingAccelerationStructure g_raytracingWorld : register(t0);		// レイトレワールド
RWTexture2D<float4> gOutPut : register(u0);								// カラー出力先

// レイ
struct RayPayload
{
	float3 color;
};

// レイ生成シェーダー
[shader("raygeneration")]
void RayGen()
{
	uint3 launchIndex = DispatchRaysIndex();
	uint3 launchDim = DispatchRaysDimensions();

	float2 crd = float2(launchIndex.xy);
	float2 dims = float2(launchDim.xy);

	float2 d = ((crd / dims) * 2.f - 1.f);
	float aspectRatio = dims.x / dims.y;

	RayDesc ray;
	ray.Origin = g_camera.pos;
	ray.Direction = normalize(float3(d.x * g_camera.aspect, -d.y, -1));
	ray.Direction = mul(g_camera.rotMat, ray.Direction);

	ray.TMin = 0;
	ray.TMax = 10000;

	RayPayload payload;
	TraceRay(
		g_raytracingWorld,
		0 /*rayFlags*/,
		0xFF,
		0 /* ray index*/,
		0,
		0,
		ray,
		payload
	);

	
	float3 col = float3(1, 0, 0);
	col = payload.color;
	gOutPut[launchIndex.xy] = float4(col, 1);

}

// レイがどのポリゴンとも接触しなかったときに呼び出されるシェーダー
[shader("miss")]
void Miss(inout RayPayload a_payload)
{
	a_payload.color = float3(0.2,0.2,0.2);
}

// レイがポリゴンにヒットしたときに呼び出されるシェーダー
[shader("closesthit")]
void Chs(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	float3 _color;
	_color.x = 1.0 - a_attr.barycentrics.x - a_attr.barycentrics.y;
	_color.y = a_attr.barycentrics.x;
	_color.z = a_attr.barycentrics.y;
	a_payload.color = _color;
}

[shader("closesthit")]
void ShadowChs(inout RayPayload a_payload,in BuiltInTriangleIntersectionAttributes a_attribs)
{
	
}

[shader("miss")]
void ShadowMiss(inout RayPayload a_payload)
{
	
}
