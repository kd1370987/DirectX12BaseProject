#include "RayShadow.hlsli"

struct RayPayload
{
	float3 color;
	int hit;
	int depth;
};

float3 ReconstructViewPos(float2 uv, float depth)
{
	float4 clip = float4(uv * 2 - 1, depth, 1);
	float4 view = mul(clip,g_camera.invProj);
	return view.xyz / view.w;
}

// 主光源が見えるか : 1 = 日なた / 0 = 影
float TraceSunVisibility(float3 a_origin, float3 a_toSun)
{
	RayDesc _ray;
	_ray.Origin = a_origin;
	_ray.Direction = a_toSun;
	_ray.TMin = 0.001f;
	_ray.TMax = 10000;

	RayPayload _payload;
	_payload.color = float3(0, 0, 0);
	_payload.depth = 0;
	_payload.hit = 0;

	TraceRay(
		g_raytracingWorld,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_FORCE_OPAQUE,
		0xFF,
		0,
		0,
		0,
		_ray,
		_payload
	);
	return _payload.color.r;
}

// 画素とフレームで変わる 0..1 のずらし(Interleaved Gradient Noise)
float InterleavedGradientNoise(float2 a_pixel, float a_frame)
{
	a_pixel += 5.588238f * a_frame;
	return frac(52.9829189f * frac(dot(a_pixel, float2(0.06711056f, 0.00583715f))));
}

//------------------------------------------------------------------------------------------
// フォグ用の影 : カメラから見えている面(空なら影の範囲の端)までの視線を歩き、
// 各点から主光源へレイを飛ばす。
//   r = 日なたの割合(歩いた点の平均) / g = 歩いた範囲の終わり(カメラからの距離)
// 範囲はビュー空間の奥行きで distance まで(シャドウマップの影の届く範囲と同じ決め方)。
// 1歩目の位置は画素ごと・フレームごとにずらし、残る縞は後ろの TAA に均させる
//------------------------------------------------------------------------------------------
void WriteVolumeShadow(uint2 a_id, float2 a_uv, float a_depth)
{
	if (g_volume.outIndex == VOLUME_SHADOW_NONE) return;
	RWTexture2D<float2> _out = ResourceDescriptorHeap[g_volume.outIndex];

	// 平行光が無い : 遮るものも無い扱い
	if (g_sun.enable == 0 || g_volume.stepCount == 0)
	{
		_out[a_id] = float2(1.0f, 0.0f);
		return;
	}

	// 視線 : 空の画素は遠い面の向きへ
	const float3 _start = g_camera.cameraPos.xyz;
	const bool _isSky = (a_depth >= 1.0f);
	float4 _clip = float4(a_uv.x * 2.0f - 1.0f, 1.0f - a_uv.y * 2.0f, _isSky ? 1.0f : a_depth, 1.0f);
	float4 _end4 = mul(_clip, g_camera.invViewProj);
	const float3 _end = _end4.xyz / _end4.w;
	const float3 _toEnd = _end - _start;
	const float3 _dir = normalize(_toEnd);
	const float _length = _isSky ? 1e30f : length(_toEnd);

	// 範囲の終わり : ビュー空間の奥行きで distance まで
	const float _viewZPerDistance = mul(float4(_dir, 0.0f), g_camera.view).z;
	if (_viewZPerDistance <= 0.0001f)
	{
		_out[a_id] = float2(1.0f, 0.0f);
		return;
	}
	const float _rangeEnd = min(_length, g_volume.distance / _viewZPerDistance);

	const float3 _toSun = normalize(-g_sun.dir);
	const float _jitter = InterleavedGradientNoise(float2(a_id), (float) (g_volume.frame % 64));
	const float _stepLength = _rangeEnd / (float) g_volume.stepCount;

	float _visibility = 0.0f;
	for (uint _i = 0; _i < g_volume.stepCount; ++_i)
	{
		const float _t = (_i + _jitter) * _stepLength;
		_visibility += TraceSunVisibility(_start + _dir * _t, _toSun);
	}

	_out[a_id] = float2(_visibility / (float) g_volume.stepCount, _rangeEnd);
}

// レイ生成シェーダー
[shader("raygeneration")]
void RayGen()
{
	uint2 _id = DispatchRaysIndex().xy;
	uint2 _dim = DispatchRaysDimensions().xy;
	
	float2 _uv = (_id + 0.5) / _dim;
	

	// GBuffer取得
	Texture2D _depthTex = ResourceDescriptorHeap[g_gbuffer.depth];
	Texture2D _normalTex = ResourceDescriptorHeap[g_gbuffer.normal];

	// 深度値を取得
	float _depth = _depthTex.Load(int3(_id, 0)).r;

	// フォグ用の影 : 面の影とは別に、全画素(空も含む)へ書く
	WriteVolumeShadow(_id, _uv, _depth);
	if (_depth >= 1.0f)
	{
		gOutPut[_id] = float4(1, 1, 1, 1);
		return;
	}
	
	// 法線を取得
	float2 _enc = _normalTex.Load(int3(_id, 0)).rg; // 法線
	float3 _normal = DecsodeNormal(_enc); // 法線を復元

	// 平行光が1つも無いシーンでは影の落としようがないので、影なし(白)で埋める。
	// 0 のまま計算に入れると向きが不定になり、影が縞になって出る
	if (g_sun.enable == 0)
	{
		gOutPut[_id] = float4(1, 1, 1, 1);
		return;
	}

	// 光源へ向かうベクトル
	float3 _lightDir = normalize(-g_sun.dir);
	
	// 光が当たらない裏面はレイを飛ばさず影(0,0,0)にする
	float _NdotL = dot(_normal, _lightDir);
	if (_NdotL <= 0.0f)
	{
		gOutPut[_id] = float4(0, 0, 0, 1);
		return;
	}
	
	
	// 3D空間での位置を復元
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, _depth, 1.0f);
	float4 _worldPos4 = mul(_clip, g_camera.invViewProj);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;
	
	// カメラ（視点）への方向ベクトルを求める
	float3 _viewVec = g_camera.cameraPos.xyz - _worldPos;
	float _dist = length(_viewVec);
	float3 _viewDir = _viewVec / _dist;

	// 球体用に輪郭に近いときには０除算を防ぐために最小値を設定
	float _Nov = max(abs(dot(_normal, _viewDir)), 0.05f);

	// 距離依存バイアスを作成
	float _biasNormal = max(0.005f, _dist * 0.0002f);
	float _biasView = max(0.01f, _dist * 0.001f) / _Nov;



	// ピクセル方向に打ち出すレイを作成する
	RayDesc _ray;
	_ray.Origin = _worldPos + _normal * _biasNormal + _viewDir * _biasView;
	_ray.Direction = _lightDir;
	_ray.TMin = 0.001f;
	_ray.TMax = 10000;


	RayPayload _payload;
	_payload.color = float3(0, 0, 0);
	_payload.depth = 0;
	_payload.hit = 0;
	
	TraceRay(
		g_raytracingWorld,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH| RAY_FLAG_SKIP_CLOSEST_HIT_SHADER| RAY_FLAG_FORCE_OPAQUE,
		0xFF,
		0,
		0,
		0,
		_ray,
		_payload
	);

	gOutPut[_id] = float4(_payload.color, 1);
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
