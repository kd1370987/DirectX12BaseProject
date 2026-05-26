//#include "../Raytracing.hlsli"

//struct GBufferIndex
//{
//	int depth;
//	int normal;
//};
//cbuffer cbGBufferIndex : register(b1)
//{
//	GBufferIndex g_gbuffer;
//}


//struct RayPayload
//{
//	int hit;
//};

//float3 ReconstructViewPos(float2 uv, float depth)
//{
//	float4 clip = float4(uv * 2 - 1, depth, 1);
//	float4 view = mul(g_camera.rotMat, clip);
//	return view.xyz / view.w;
//}

//// レイ生成シェーダー
//[shader("raygeneration")]
//void RayGen()
//{
//	uint2 _id = DispatchRaysIndex().xy;
//	uint2 _dim = DispatchRaysDimensions().xy;
	
//	float2 _uv = (_id + 0.5) / _dim;
//	_uv = _uv * 2.0 - 1.0;

//	_uv.x *= g_camera.aspect;

//	float3 _dir = normalize(float3(_uv.x, -_uv.y, 1.0f));

//	// 頂点バッファとインデックスバッファを取得
//	InstanceData _instance = g_instanceData[InstanceID()];
	
//	// GBuffer取得
//	Texture2D _depthTex = ResourceDescriptorHeap[g_gbuffer.depth];
//	Texture2D _normalTex = ResourceDescriptorHeap[g_gbuffer.normal];

//	// 深度値を取得
//	float _depth = _depthTex.Load(int3(_id, 0));
//	// 法線を取得
//	float3 normal = normalize(_normalTex.Load(int3(_id, 0)).xyz);

//	// 3D空間での位置を復元
//	float3 _viewPos = ReconstructViewPos(_uv, _depth);
//	float4 _worldPos4 = mul(float4(_viewPos, 1), cViewInv);
//	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;
	
//	// ピクセル方向に打ち出すレイを作成する
//	RayDesc _ray;
//	_ray.Origin = g_camera.pos;
//	_ray.Direction = mul((float3x3) g_camera.rotMat, _dir);
//	_ray.TMin = 0.001;
//	_ray.TMax = 10000;


//	RayPayload _payload;
//	_payload.hit = 0;
	
//	TraceRay(
//		g_raytracingWorld,
//		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
//		0xFF,
//		0,
//		1,
//		0,
//		_ray,
//		_payload
//	);

	
//	gOutPut[_id] = _payload.hit ? float4(0,0,0,1) : float4(1,1,1,1);

//}

//[shader("anyhit")]
//void ShadowAnyHit(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
//{
//	a_payload.hit = 1;
//	AcceptHitAndEndSearch();
//}

//[shader("miss")]
//void ShadowMiss(inout RayPayload a_payload)
//{
//	a_payload.hit = 0;
//}
