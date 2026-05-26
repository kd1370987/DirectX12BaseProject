// カメラの定数バッファ
struct Camera
{
	float4x4 rotMat; // カメラの回転行列
	float3 pos; // カメラの座標
	float aspect; // カメラのアスペクト比
	float farClip; // 遠平面
	float nearClip; // 近平面
};
cbuffer cbCamera : register(b0)
{
	Camera g_camera;
}
struct GBufferIndex
{
	int depth;
	int normal;
};
cbuffer cbGBufferIndex : register(b1)
{
	GBufferIndex g_gbuffer;
}
struct InstanceData
{
	uint vertexIdx; // SRV
	uint indexIdx; // SRV

	// このメッシュのマテリアル群がg_materialDataの何番目から始まるか
	uint materialOffset;
	uint pad;
};
struct Vertex
{
	float3 pos;
	float3 normal;
	float2 uv;
	float3 tangent;
	float4 color;
};

RaytracingAccelerationStructure g_raytracingWorld : register(t0);	// レイトレワールド
RWTexture2D<float4> gOutPut : register(u0);							// カラー出力先
StructuredBuffer<InstanceData> g_instanceData : register(t1); // インスタンスごとのデータ
sampler gSamp : register(s0);

struct RayPayload
{
	int hit;
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

	float3 _dir = normalize(float3(_uv.x, -_uv.y, 1.0f));

	// GBuffer取得
	Texture2D _depthTex = ResourceDescriptorHeap[g_gbuffer.depth];
	Texture2D _normalTex = ResourceDescriptorHeap[g_gbuffer.normal];
	
	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	_ray.Origin = g_camera.pos;
	_ray.Direction = mul((float3x3) g_camera.rotMat, _dir);
	_ray.TMin = 0.001;
	_ray.TMax = 10000;


	RayPayload _payload;
	_payload.hit = 0;
	
	TraceRay(
		g_raytracingWorld,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0xFF,
		0,
		1,
		0,
		_ray,
		_payload
	);

	
	gOutPut[_id] = _payload.hit ? float4(0,0,0,1) : float4(1,1,1,1);

}

[shader("anyhit")]
void ShadowAnyHit(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.hit = 1;
	AcceptHitAndEndSearch();
}

[shader("miss")]
void ShadowMiss(inout RayPayload a_payload)
{
	a_payload.hit = 0;
}
