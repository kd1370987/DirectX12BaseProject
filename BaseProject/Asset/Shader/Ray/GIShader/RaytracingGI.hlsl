#include "../Raytracing.hlsli"
#include "../../Source/CalcNormal.hlsli"
#include "../../Common/RootParameters/AmbientData.hlsli"
struct InstanceData
{
	uint vertexIdx; // SRV
	uint indexIdx; // SRV

	// このメッシュのマテリアル群がg_materialDataの何番目から始まるか
	uint materialOffset;
	uint pad;
};
// マテリアル構造体
struct Material
{
	float4 baseColor;
	float3 emissive;
	float metallic;
	
	float roughness;
	int baseIndex;
	int metaRoughnessIndex;
	int emissiveIndex;
	
	int normalIndex;
	uint startIndexLocation; // このサブメッシュのインデックスバッファが何番目から始まるか
	float2 pad;
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
StructuredBuffer<InstanceData> g_instanceData : register(t1); // インスタンスごとのデータ
StructuredBuffer<Material> g_materialData : register(t2); // インスタンスごとのデータ

struct GBufferIndex
{
	int depth;
	int normal;

	int frameCount;
};
cbuffer cbGBufferIndex : register(b1)
{
	GBufferIndex g_gbuffer;
}


// UV座標を取得
float2 GetUV(BuiltInTriangleIntersectionAttributes a_attribs, InstanceData instance, uint primID, Material material)
{
	float3 _barycentrics = float3(1.0 - a_attribs.barycentrics.x - a_attribs.barycentrics.y, a_attribs.barycentrics.x, a_attribs.barycentrics.y);

	// メインヒープから、このインスタンスのインデックスバッファと頂点バッファを直接取得
	StructuredBuffer<int> indexBuff = ResourceDescriptorHeap[instance.indexIdx];
	StructuredBuffer<Vertex> vertexBuff = ResourceDescriptorHeap[instance.vertexIdx];

	// プリミティブIDではなくサブメッシュ番号を取得する
	uint _baseIndexLocation = material.startIndexLocation + (primID * 3);

	// プリミティブインデックスから頂点番号を取得
	uint _v0 = indexBuff[_baseIndexLocation];
	uint _v1 = indexBuff[_baseIndexLocation + 1];
	uint _v2 = indexBuff[_baseIndexLocation + 2];

	// UV取得
	float2 _uv0 = vertexBuff[_v0].uv;
	float2 _uv1 = vertexBuff[_v1].uv;
	float2 _uv2 = vertexBuff[_v2].uv;

	float2 _uv = _barycentrics.x * _uv0 + _barycentrics.y * _uv1 + _barycentrics.z * _uv2;
	return _uv;
}
// 法線の取得
float3 GetNormal(BuiltInTriangleIntersectionAttributes a_attribs, float2 a_uv, InstanceData instance, uint primID, Material material)
{
	float3 _barycentrics = float3(1.0 - a_attribs.barycentrics.x - a_attribs.barycentrics.y, a_attribs.barycentrics.x, a_attribs.barycentrics.y);
	
	// 同様にメインヒープから取得
	StructuredBuffer<int> indexBuff = ResourceDescriptorHeap[instance.indexIdx];
	StructuredBuffer<Vertex> vertexBuff = ResourceDescriptorHeap[instance.vertexIdx];
	
	// プリミティブIDではなくサブメッシュ番号を取得する
	uint _baseIndexLocation = material.startIndexLocation + (primID * 3);
	
	uint _v0 = indexBuff[_baseIndexLocation];
	uint _v1 = indexBuff[_baseIndexLocation + 1];
	uint _v2 = indexBuff[_baseIndexLocation + 2];
	
	// 法線取得
	float3 _n0 = vertexBuff[_v0].normal;
	float3 _n1 = vertexBuff[_v1].normal;
	float3 _n2 = vertexBuff[_v2].normal;
	float3 _normal = normalize(_barycentrics.x * _n0 + _barycentrics.y * _n1 + _barycentrics.z * _n2);

	// タンジェント
	float3 _t0 = vertexBuff[_v0].tangent;
	float3 _t1 = vertexBuff[_v1].tangent;
	float3 _t2 = vertexBuff[_v2].tangent;
	float3 _tangent = normalize(_barycentrics.x * _t0 + _barycentrics.y * _t1 + _barycentrics.z * _t2);

	// ビノーマルを計算
	float3 _binormal = normalize(cross(_tangent, _normal));

	// 法線マップもマテリアルに仕込んだベースインデックスから取得
	Texture2D normalTex = ResourceDescriptorHeap[material.normalIndex];
	float3 _binSpaceNormal = normalTex.SampleLevel(gSamp, a_uv, 0).rgb;
	_binSpaceNormal = (_binSpaceNormal * 2.0f) - 1.0f;

	// タンジェント空間からワールド空間に変換
	_normal = _tangent * _binSpaceNormal.x + _binormal * _binSpaceNormal.y + _normal * _binSpaceNormal.z;
	_normal = normalize(mul(_normal, (float3x3) WorldToObject3x4()));
	return _normal;
}

// レイ
struct RayPayload
{
	float3 color;
	int hit;
	int depth;
	uint seed; // 乱数状態を引き継ぐ
	float hitDistance;
};

// 光源に向かってレイを飛ばす
void TraceLightRay(inout RayPayload a_rayPayload, float3 a_normal, float3 a_geoNormal)
{
	float _hitT = RayTCurrent(); // レイが当たった距離
	float3 _rayDirW = WorldRayDirection(); // レイのワールド空間での方向
	float3 _rayOriginW = WorldRayOrigin(); // レイのワールド空間での開始位置
	float3 _posW = _rayOriginW + _hitT * _rayDirW; // レイが当たった位置を計算
	float3 _ligDir = normalize(-g_ambient.DL_Dir); // 光の方向

	// 光源の方向にレイを飛ばす
	RayDesc _ray;
	// 法線方向に少し浮かせたあとに、光方向にも少し引っ張り上げ
	float _normalBias = 0.00001f;
	float _lightBias = 0.0;
	_ray.Origin = _posW + a_geoNormal * _normalBias;
	_ray.Direction = _ligDir;
	_ray.TMin = 0.0001f;
	_ray.TMax = 10000;
	
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
void TraceIndirectRay(inout RayPayload a_rayPayload, float3 a_normal)
{
	if (a_rayPayload.depth >= 3)
	{
		return;
	}
	
	float _hitT = RayTCurrent(); // レイが当たった距離
	float3 _rayDirW = WorldRayDirection(); // レイのワールド空間での方向
	float3 _rayOriginW = WorldRayOrigin(); // レイのワールド空間での開始位置
	float3 _posW = _rayOriginW + _hitT * _rayDirW;

	// ペイロードからシードを取り出し、乱数を２つ作って次のシードを更新する
	float2 _rand = float2(Random01(a_rayPayload.seed), Random01(a_rayPayload.seed * 17 + 3));
	a_rayPayload.seed = Hash(a_rayPayload.seed); // シードを更新

	// 法線ベースの半球ランダム方向を取得
	float3 _diffuseDir = SampleHemisphereCosine(a_normal, _rand);

	// ペイロードを作成
	RayPayload _refPayload;
	_refPayload.color = float3(0, 0, 0);
	_refPayload.depth = a_rayPayload.depth;
	_refPayload.hit = 0;
	_refPayload.seed = a_rayPayload.seed; // 更新したシードを渡す
	
	// 反射レイを飛ばす
	RayDesc _ray;
	_ray.Origin = _posW + a_normal * 0.0001f;
	_ray.Direction = _diffuseDir; // ランダムな方向に飛ばす
	_ray.TMin = 0.001f;
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
	
	// GBuffer取得
	Texture2D _depthTex = ResourceDescriptorHeap[g_gbuffer.depth];
	Texture2D _normalTex = ResourceDescriptorHeap[g_gbuffer.normal];

	// 深度値を取得
	float _depth = _depthTex.Load(int3(_id, 0)).r;
	if (_depth >= 1.0f)
	{
		gOutPut[_id] = float4(1, 1, 1, 1);
		return;
	}
	
	// 法線を取得
	float2 _enc = _normalTex.Load(int3(_id, 0)).rg; // 法線
	float3 _normal = DecsodeNormal(_enc); // 法線を復元

	// 3D空間での位置を復元
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, _depth, 1.0f);
	float4 _worldPos4 = mul(_clip, g_camera.invViewProj);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;

	// ピクセル座標からサーフェイス面上で阪急範囲にランダムにレイを飛ばす
	// ピクセル位置と時間をXORで混ぜてからハッシュ化する
	uint pixelIndex = _id.y * _dim.x + _id.x;
	uint _seed = Hash(pixelIndex ^ (g_gbuffer.frameCount * 2654435769u));

	// レイ構造体を作成
	RayDesc _ray;
	_ray.Origin = _worldPos + _normal * 0.0001f;
	_ray.Direction = SampleHemisphereCosine(_normal, float2(Random01(_seed), Random01(_seed * 17 + 3)));
	_ray.TMin = 0.001;
	_ray.TMax = 1000;

	// ペイロードを作成
	RayPayload _payload;
	_payload.color = float3(0, 0, 0);
	_payload.depth = 0;
	_payload.hit = 0;
	_payload.seed = Hash(_seed);
	_payload.hitDistance = -1.0f;

	// レイ発射
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

	// 出力
	gOutPut[_id] = float4(_payload.color, _payload.hitDistance);
}

// レイがどのポリゴンとも接触しなかったときに呼び出されるシェーダー
[shader("miss")]
void Miss(inout RayPayload a_payload)
{
	a_payload.color = float3(0.2, 0.2, 0.3);
	a_payload.hitDistance = -1.0f;				// 当たらなかったらマイナス
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

	// 今のレイが飛んだ距離をペイロードに保存
	a_payload.hitDistance = RayTCurrent();
	
	// 各種IDとデータの取得
	uint instID = InstanceID(); // インスタンス番号
	uint geomID = GeometryIndex(); // 当たったサブメッシュ番号
	uint primID = PrimitiveIndex(); // 当たったポリゴン番号

	// データを配列から取得
	InstanceData instance = g_instanceData[instID]; // インスタンス情報
	Material _material = g_materialData[instance.materialOffset + geomID]; // サブメッシュマテリアル情報
	
	// 頂点バッファとインデックスバッファを取得
	StructuredBuffer<int> indexBuff = ResourceDescriptorHeap[instance.indexIdx];
	StructuredBuffer<Vertex> vertexBuff = ResourceDescriptorHeap[instance.vertexIdx];
	uint _baseIndexLocation = _material.startIndexLocation + (primID * 3);
	uint _v0 = indexBuff[_baseIndexLocation];
	uint _v1 = indexBuff[_baseIndexLocation + 1];
	uint _v2 = indexBuff[_baseIndexLocation + 2];

	// 頂点の位置（オブジェクト空間）を取得
	float3 _vpos0 = vertexBuff[_v0].pos;
	float3 _vpos1 = vertexBuff[_v1].pos;
	float3 _vpos2 = vertexBuff[_v2].pos;

	// オブジェクト空間からワールド空間へ変換（ObjectToWorld4x3()を使用）
	// ObjectToWorld4x3()がオブジェクトからワールドへの変換行列を返す
	_vpos0 = mul(float4(_vpos0, 1.0), ObjectToWorld4x3());
	_vpos1 = mul(float4(_vpos1, 1.0), ObjectToWorld4x3());
	_vpos2 = mul(float4(_vpos2, 1.0), ObjectToWorld4x3());
	

	// 外積を使ってジオメトリ法線（ポリゴン平面の法線）を計算
	float3 _geoNormal = normalize(cross(_vpos1 - _vpos0, _vpos2 - _vpos0));
	// UV取得 (引数を追加)
	float2 _uv = GetUV(a_attr, instance, primID, _material);

	// 法線取得 (引数を追加)
	float3 _normal = GetNormal(a_attr, _uv, instance, primID, _material);

	// 光源に向かってレイを飛ばす
	TraceLightRay(a_payload, _normal, _geoNormal);
	float _lig = 0.0f;
	// 直接光が当たってる
	if (a_payload.hit == 0)
	{
		float3 _ligDir = normalize(-g_ambient.DL_Dir);
		// ジオメトリ法線がライトに対して背を向けている場合強制的に影にする
		float _GdotL = dot(_geoNormal, _ligDir);
		if (_GdotL > 0.0f)
		{
			_lig = max(0, dot(_normal, _ligDir));
		}
		else
		{
			_lig = 0.0f;
		}

	}
	
	// 間接光の取得
	RayPayload _refPayload;
	_refPayload.color = float3(0,0,0);
	_refPayload.depth = a_payload.depth;
	_refPayload.seed = a_payload.seed;
	TraceIndirectRay(_refPayload, _normal); // 次のバウンスレイを飛ばす

	// テクスチャからのアルベド取得
	float dist = RayTCurrent();
	float lod = clamp(log2(dist * 0.5), 0, 5);
	Texture2D albedoTex = ResourceDescriptorHeap[_material.baseIndex];
	Texture2D metaRogTex = ResourceDescriptorHeap[_material.metaRoughnessIndex];
	float3 _albedo = albedoTex.SampleLevel(gSamp, _uv, lod).rgb * _material.baseColor.xyz;

	// 最終的な色の合成
	float3 _directLight = (_lig / 3.141592f) * g_ambient.DL_Color; // ライトの色をかける
	float3 _indirectLight = _refPayload.color; // 飛んだ先から持ち帰ってきた色

	// アルベド * (直接光 + 間接光) + エミッシブ(自己発光)
	float3 _finalColor = _albedo * (_directLight + _indirectLight) + _material.emissive;

	// RGBの計算結果がマイナスや無限大（NaN）になるのを防ぐ
	a_payload.color = clamp(_finalColor, 0.0f, 10.0f);
	
	// ペイロードの深度とシードを親に渡す
	a_payload.seed = _refPayload.seed;
	a_payload.depth--;
}

[shader("closesthit")]
void ShadowCHS(inout RayPayload a_payload, in BuiltInTriangleIntersectionAttributes a_attr)
{
	a_payload.hit = 1;
}


