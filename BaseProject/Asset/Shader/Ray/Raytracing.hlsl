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
	uint2 _id = DispatchRaysIndex().xy;
	uint2 _dim = DispatchRaysDimensions().xy;
	
	float2 _uv = (_id + 0.5) / _dim;
	_uv = _uv * 2.0 - 1.0;

	_uv.x *= g_camera.aspect;

	float3 _dir = normalize(float3(_uv.x,-_uv.y,-1.0f));

	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	//_ray.Origin = g_camera.pos;
	_ray.Origin = float3(0,0,-5);
	//_ray.Direction = mul((float3x3)g_camera.rotMat,_dir);
	_ray.Direction = float3(0,0,-1);
	_ray.TMin = 0.001;
	_ray.TMax = 10000;


	RayPayload _payload;
	_payload.color = float3(0,0,0);
	
	TraceRay(
		g_raytracingWorld,
		0,
		0xFF,
		0,
		1,
		0,
		_ray,
		_payload
	);

	float3 _col = _payload.color;

	gOutPut[_id] = float4(_col,1);
}

// レイがどのポリゴンとも接触しなかったときに呼び出されるシェーダー
[shader("miss")]
void Miss(inout RayPayload a_payload)
{
	a_payload.color = float3(0.2,0.2,1.0);
}

// レイがポリゴンにヒットしたときに呼び出されるシェーダー
[shader("closesthit")]
void ClosestHit(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	float3 _color;
	_color.x = 1.0 - a_attr.barycentrics.x - a_attr.barycentrics.y;
	_color.y = a_attr.barycentrics.x;
	_color.z = a_attr.barycentrics.y;
	
	a_payload.color = _color;
	a_payload.color = float3(1, 0, 0);

}
