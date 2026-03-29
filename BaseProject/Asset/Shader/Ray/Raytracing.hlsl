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

struct InstanceData
{
	uint vertexIdx;		// SRV
	uint indexIdx;		// SRV
};

// 頂点構造体
struct Vertex
{
	float2 uv;
	float2 pad;
};

struct Material
{
	float4 baseColor;
	int baseTexSRV;
};

RaytracingAccelerationStructure g_raytracingWorld : register(t0);		// レイトレワールド
RWTexture2D<float4> gOutPut : register(u0);								// カラー出力先
StructuredBuffer<InstanceData> g_instanceData : register(t1);							// インスタンスごとのデータ
StructuredBuffer<Material> g_materialData : register(t2);							// インスタンスごとのデータ

sampler gSamp : register(s0);

Texture2D g_albedoTex : register(t3);
Texture2D g_metaRogTex : register(t4);
Texture2D g_emiTex : register(t5);
Texture2D g_normalTex : register(t6);

StructuredBuffer<int> g_indexBuff : register(t7);
StructuredBuffer<Vertex> g_vertexBuff : register(t8);


// レイ
struct RayPayload
{
	float3 color;
};


// UV座標を取得
float2 GetUV(BuiltInTriangleIntersectionAttributes a_attribs)
{
	float3 _barycentrics =float3(1.0 - a_attribs.barycentrics.x - a_attribs.barycentrics.y, a_attribs.barycentrics.x, 
	a_attribs.barycentrics.y);

	// プリミティブIDを取得
	uint _primID = PrimitiveIndex();

	// プリミティブインデックスから頂点番号を取得
	uint _v0 = g_indexBuff[_primID * 3];
	uint _v1 = g_indexBuff[_primID * 3 + 1];
	uint _v2 = g_indexBuff[_primID * 3 + 2];

	// UV取得

	float2 _uv0 = g_vertexBuff[_v0].uv;
	float2 _uv1 = g_vertexBuff[_v1].uv;
	float2 _uv2 = g_vertexBuff[_v2].uv;

	// UVを計算して返す

	float2 _uv = _barycentrics.x * _uv0 + _barycentrics.y * _uv1 + _barycentrics.z * _uv2;
	return _uv;
}


// レイ生成シェーダー
[shader("raygeneration")]
void RayGen()
{
	uint2 _id = DispatchRaysIndex().xy;
	uint2 _dim = DispatchRaysDimensions().xy;
	
	float2 _uv = (_id + 0.5) / _dim;
	_uv = _uv * 2.0 - 1.0;

	_uv.x *= g_camera.aspect;

	float3 _dir = normalize(float3(_uv.x,-_uv.y,1.0f));

	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	_ray.Origin = g_camera.pos;
	_ray.Direction = mul((float3x3)g_camera.rotMat,_dir);
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
	float3 _color = float3(0,0,0);
	_color.x = 1.0 - a_attr.barycentrics.x - a_attr.barycentrics.y;
	_color.y = a_attr.barycentrics.x;
	_color.z = a_attr.barycentrics.y;

	// UV
	float3 _barycentrics = float3(1.0 - a_attr.barycentrics.x - a_attr.barycentrics.y, a_attr.barycentrics.x,
	a_attr.barycentrics.y);

	// プリミティブIDを取得
	uint _primID = PrimitiveIndex();

	// プリミティブインデックスから頂点番号を取得
	uint _v0 = g_indexBuff[_primID * 3];
	uint _v1 = g_indexBuff[_primID * 3 + 1];
	uint _v2 = g_indexBuff[_primID * 3 + 2];

	// UV取得
	float2 _uv = GetUV(a_attr);
	
	
	// インスタンスごとの情報を取得
	Material _material = g_materialData[InstanceID()];
	_color = g_albedoTex.SampleLevel(gSamp, _uv,0).rgb;
	//_color = g_normalTex.SampleLevel(gSamp, _uv,0).rgb;

	//_color = float3(_uv,0);
	//_color = float3(_v0,_v1,_v2);
	//_color = float3(g_vertexBuff[0].uv,0);

	//_color = float3(_v0 % 10 / 10.0, _v1 % 10 / 10.0, _v2 % 10 / 10.0);
	
	a_payload.color = _color;
}
