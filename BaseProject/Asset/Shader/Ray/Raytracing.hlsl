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
	float3 pos;
	float3 normal;
	float2 uv;
	float3 tangent;
	float4 color;
};

// マテリアル構造体
struct Material
{
	float4 baseColor;
	float metallic;
	float roughness;
	float3 emissive;

	int baseIndex;
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
	int hit;
	int depth;
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

// 法線の取得
float3 GetNormal(BuiltInTriangleIntersectionAttributes a_attribs, float2 a_uv)
{
	float3 _barycentrics =float3(1.0 - a_attribs.barycentrics.x - a_attribs.barycentrics.y, a_attribs.barycentrics.x, 
	a_attribs.barycentrics.y);
	// プリミティブIDを取得
	uint _primID = PrimitiveIndex();
	// プリミティブインデックスから頂点番号を取得
	uint _v0 = g_indexBuff[_primID * 3];
	uint _v1 = g_indexBuff[_primID * 3 + 1];
	uint _v2 = g_indexBuff[_primID * 3 + 2];
	
	// 法線取得
	float3 _n0 = g_vertexBuff[_v0].normal;
	float3 _n1 = g_vertexBuff[_v1].normal;
	float3 _n2 = g_vertexBuff[_v2].normal;
	// 法線を計算
	float3 _normal = _barycentrics.x * _n0 + _barycentrics.y * _n1 + _barycentrics.z * _n2;
	_normal = normalize(_normal);

	// タンジェント
	float3 _t0 = g_vertexBuff[_v0].tangent;
	float3 _t1 = g_vertexBuff[_v1].tangent;
	float3 _t2 = g_vertexBuff[_v2].tangent;
	// タンジェントを計算
	float3 _tangent = _barycentrics.x * _t0 + _barycentrics.y * _t1 + _barycentrics.z * _t2;
	_tangent = normalize(_tangent);

	// ビノーマルを計算
	float3 _binormal = normalize(cross(_tangent, _normal));

	// 法線マップから法線を取得
	float3 _binSpaceNormal = g_normalTex.SampleLevel(gSamp, a_uv, 0).rgb;
	_binSpaceNormal = (_binSpaceNormal * 2.0f) - 1.0f;

	// タンジェント空間からワールド空間に変換
	_normal = _tangent * _binSpaceNormal.x + _binormal * _binSpaceNormal.y + _normal * _binSpaceNormal.z;
	return _normal;
}

// 光源に向かってレイを飛ばす
void TraceLightRay(inout RayPayload a_rayPayload,float3 a_normal)
{
	float _hitT = RayTCurrent();			// レイが当たった距離
	float3 _rayDirW = WorldRayDirection();	// レイのワールド空間での方向
	float3 _rayOriginW = WorldRayOrigin();	// レイのワールド空間での開始位置

	// レイが当たった位置を計算
	float3 _posW = _rayOriginW + _hitT * _rayDirW;

	// 光源の方向にレイを飛ばす
	RayDesc _ray;
	_ray.Origin = _posW;
	_ray.Direction = normalize(float3(0.5, 0.5, 0.2));
	_ray.TMin = 0.01f;
	_ray.TMax = 100;
	
	TraceRay(
		g_raytracingWorld,	// TLAS
		0,					// RayFlags
		0xFF,				// InstanceInclusionMask
		1,					// RayContributionToHitGroupIndex
		2,					// MultiplierForGeometryContributionToHitGroupIndex
		1,					// MissShaderIndex
		_ray,				// Ray
		a_rayPayload		// RayPayload
	);
}

// 反射レイを飛ばす
void TraceReflectionRay(inout RayPayload a_rayPayload, float3 a_normal)
{
	if (a_rayPayload.depth >= 3)
	{
		return;
	}
	
	
	float _hitT = RayTCurrent(); // レイが当たった距離
	float3 _rayDirW = WorldRayDirection(); // レイのワールド空間での方向
	float3 _rayOriginW = WorldRayOrigin(); // レイのワールド空間での開始位置

		// 反射ベクトルを計算
	float3 _reflectDir = reflect(_rayDirW, a_normal);

		// 反射レイの情報をセット
	float3 _posW = _rayOriginW + _hitT * _rayDirW;

	RayPayload _refPayload;
	_refPayload.color = float3(0, 0, 0);
	_refPayload.depth = a_rayPayload.depth;
	_refPayload.hit = 0;
	
	// 反射レイを飛ばす
	RayDesc _ray;
	_ray.Origin = _posW + a_normal * 0.1f;
	//_ray.Origin = _posW;
	_ray.Direction = _reflectDir;
	_ray.TMin = 0.01f;
	_ray.TMax = 1000;

	TraceRay(
			g_raytracingWorld,
			0,
			0xFF,
			0,
			2,
			0,
			_ray,
			_refPayload
		);

	a_rayPayload.color = _refPayload.color;
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
	_payload.depth = 0;
	_payload.hit = 0;
	
	TraceRay(
		g_raytracingWorld,
		0,
		0xFF,
		0,
		2,
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

[shader("miss")]
void ShadowMiss(inout RayPayload a_payload)
{
	a_payload.hit = 0;
}

// レイがポリゴンにヒットしたときに呼び出されるシェーダー
[shader("closesthit")]
void ClosestHit(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.depth++;
	
	// UV取得
	float2 _uv = GetUV(a_attr);

	// 法線取得
	float3 _normal = GetNormal(a_attr, _uv);

	// 光源に向かってレイを飛ばす
	TraceLightRay(a_payload,_normal);
	float _lig = 0.0f;
	if (a_payload.hit == 0)
	{
		float3 _ligDir = normalize(float3(0.5, 0.5, 0.2));
		float _t = max(0, dot(_normal, _ligDir));
		_lig = _t;
	}

	// 観光光
	_lig += 0.5f;
	RayPayload _refPayload;
	_refPayload.depth = a_payload.depth;
	_refPayload.color = float3(0, 0, 0);

	// 反射レイを飛ばす
	TraceReflectionRay(_refPayload, _normal);

	
	float dist = RayTCurrent();
	float lod = log2(dist * 0.5); // 調整必要
	lod = clamp(lod, 0, 5);
	
	// このプリミティブの反射率を取得
	Material _material = g_materialData[InstanceID()];
	float _reflectRate = g_metaRogTex.SampleLevel(gSamp, _uv, lod).b * _material.metallic;
	float3 _color = g_albedoTex.SampleLevel(gSamp, _uv, lod).rgb;

	//float3 _baseTex = ResourceDescriptorHeap[4].SampleLevel(gSamp, _uv, lod).rgb;
	Texture2D _baseTex = (Texture2D) ResourceDescriptorHeap[4];
	_color = _baseTex.SampleLevel(gSamp, _uv, lod).rgb;
	//_color = _baseTex;
	
	_color *= _lig;
	a_payload.color = lerp(_color,_refPayload.color,_reflectRate);
	
	a_payload.depth--;
}

[shader("closesthit")]
void ShadowCHS(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.hit = 1;
}


