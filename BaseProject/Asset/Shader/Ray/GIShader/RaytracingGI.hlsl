#include "../Raytracing.hlsli"

// レイ
struct RayPayload
{
	float3 color;
	int hit;
	int depth;
};

// 光源に向かってレイを飛ばす
void TraceLightRay(inout RayPayload a_rayPayload, float3 a_normal)
{
	float _hitT = RayTCurrent(); // レイが当たった距離
	float3 _rayDirW = WorldRayDirection(); // レイのワールド空間での方向
	float3 _rayOriginW = WorldRayOrigin(); // レイのワールド空間での開始位置

	// レイが当たった位置を計算
	float3 _posW = _rayOriginW + _hitT * _rayDirW;

	// 光源の方向にレイを飛ばす
	RayDesc _ray;
	_ray.Origin = _posW;
	_ray.Direction = normalize(float3(0.5, 0.5, 0.2));
	_ray.TMin = 0.01f;
	_ray.TMax = 100;
	
	TraceRay(
		g_raytracingWorld, // TLAS
		0, // RayFlags
		0xFF, // InstanceInclusionMask
		1, // RayContributionToHitGroupIndex
		0, // MultiplierForGeometryContributionToHitGroupIndex
		1, // MissShaderIndex
		_ray, // Ray
		a_rayPayload // RayPayload
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
			0,
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

	// UVを求める
	float2 _uv = (_id + 0.5) / _dim;
	
	// NDC(正規化デバイス座標)空間(-1.0f～1.0)へ変換
	// HLSLの座標系に合わせるため、Y軸を反転
	float4 _targetNDC = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, 1.0f, 1.0f);

	// 新しい逆ビュープロジェクション行列を使って、ワールド空間でのターゲット位置を計算
	float4 _worldTarget = mul(g_camera.invViewProj, _targetNDC);
	_worldTarget.xyz /= _worldTarget.w;		// 投資除算
	
	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	_ray.Origin = g_camera.pos;
	_ray.Direction = normalize(_worldTarget.xyz - _ray.Origin);
	_ray.TMin = 0.001;
	_ray.TMax = 10000;


	RayPayload _payload;
	_payload.color = float3(0, 0, 0);
	_payload.depth = 0;
	_payload.hit = 0;
	
	TraceRay(
		g_raytracingWorld,
		0,
		0xFF,
		0,
		0,
		0,
		_ray,
		_payload
	);

	float3 _col = _payload.color;

	gOutPut[_id] = float4(_col, 1);
}

// レイがどのポリゴンとも接触しなかったときに呼び出されるシェーダー
[shader("miss")]
void Miss(inout RayPayload a_payload)
{
	a_payload.color = float3(0.2, 0.2, 1.0);
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
	
	// 各種IDとデータの取得
	uint instID = InstanceID();		// インスタンス番号
	uint geomID = GeometryIndex();	// 当たったサブメッシュ番号
	uint primID = PrimitiveIndex();	// 当たったポリゴン番号

	// データを配列から取得
	InstanceData instance = g_instanceData[instID]; // インスタンス情報
	Material _material = g_materialData[instance.materialOffset + geomID];	// サブメッシュマテリアル情報

	// UV取得 (引数を追加)
	float2 _uv = GetUV(a_attr, instance, primID,_material);

	// 法線取得 (引数を追加)
	float3 _normal = GetNormal(a_attr, _uv, instance, primID, _material);

	// 光源に向かってレイを飛ばす
	TraceLightRay(a_payload, _normal);
	float _lig = 0.0f;
	if (a_payload.hit == 0)
	{
		float3 _ligDir = normalize(float3(0.5, 0.5, 0.2));
		_lig = max(0, dot(_normal, _ligDir));
	}

	_lig += 0.5f;
	RayPayload _refPayload;
	_refPayload.depth = a_payload.depth;
	_refPayload.color = float3(0, 0, 0);

	// 反射レイを飛ばす
	TraceReflectionRay(_refPayload, _normal);
	
	float dist = RayTCurrent();
	float lod = clamp(log2(dist * 0.5), 0, 5);
	
	// テクスチャをメインヒープのインデックスから直接サンプリング
	Texture2D albedoTex = ResourceDescriptorHeap[_material.baseIndex];
	Texture2D metaRogTex = ResourceDescriptorHeap[_material.metaRoughnessIndex];

	float _reflectRate = metaRogTex.SampleLevel(gSamp, _uv, lod).b * _material.metallic;
	float3 _color = albedoTex.SampleLevel(gSamp, _uv, lod).rgb;

	_color *= _lig;
	a_payload.color = lerp(_color, _refPayload.color, _reflectRate);
	
	a_payload.depth--;
}

[shader("closesthit")]
void ShadowCHS(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.hit = 1;
}


