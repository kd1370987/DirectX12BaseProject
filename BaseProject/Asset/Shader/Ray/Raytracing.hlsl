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
	uint3 _launchIndex = DispatchRaysIndex();
	uint3 _launchDim = DispatchRaysDimensions();

	float2 _crd = float2(_launchIndex.xy);
	float2 _dims = float2(_launchDim.xy);

	float2 _d = ((_crd / _dims) * 2.f - 1.f);
	float _aspectRatio = _dims.x / _dims.y;

	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	_ray.Origin = g_camera.pos;
	_ray.Direction = normalize(float3(_d.x * g_camera.aspect, -_d.y, -1));
	_ray.Direction = mul((float3x3)g_camera.rotMat,_ray.Direction);

	_ray.TMin = 0;
	_ray.TMax = 10000;


	RayPayload _payload;
	_payload.color = float3(0,0,0);
	
	TraceRay(g_raytracingWorld,0,0xFF,0,0,0,_ray,_payload);

	float3 _col = _payload.color;

	gOutPut[_launchIndex.xy] = float4(_col,1);
}

// レイがどのポリゴンとも接触しなかったときに呼び出されるシェーダー
[shader("miss")]
void Miss(inout RayPayload a_payload)
{
	a_payload.color = float3(0.2,0.2,0.4);
}

// レイがポリゴンにヒットしたときに呼び出されるシェーダー
[shader("closesthit")]
void ClosestHit(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.color = float3(1, 0, 0);

}
