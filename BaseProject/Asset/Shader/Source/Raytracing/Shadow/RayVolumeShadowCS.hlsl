//==========================================================================================
//
// RayVolumeShadowCS
//
// フォグ用の影 : カメラから見えている面(空なら影の範囲の端)までの視線を歩き、
// 各点から主光源へレイを飛ばす。
//   r = 日なたの割合(歩いた点の平均) / g = 歩いた範囲の終わり(カメラからの距離)
// 範囲はビュー空間の奥行きで distance まで(シャドウマップの影の届く範囲と同じ決め方)。
//
// ・フォグの影は低周波なので、低解像度(既定 1/4)で回す。
//   引き伸ばしはフォグ(SceneVolumetricFogCS)が g を見ながら行う
// ・レイは可視判定だけなので、DispatchRays ではなくインラインレイトレ(RayQuery)で飛ばす。
//   シェーダーテーブルもペイロードも要らない
// ・1歩目の位置は画素ごと・フレームごとにずらし、残る縞は後ろの TAA に均させる
//
//==========================================================================================
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/LightData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)          カメラ
//   1 : CBV(b1)          主光源(平行光の向き)
//   2 : CBV(b2)          歩く範囲・歩数
//   3 : SRVの番号      シーンの深度(フル解像度。レンダーグラフが張る)
//   4 : UAVの番号      フォグ用の影(低解像度。レンダーグラフが張る)
//   5 : SRV(t0)          TLAS(パスが張る)
//
// 実体は C++ 側(RaytracingVolumeShadowPass)。並びを変えるときは両方を揃えること
//==========================================================================================
#define RAY_VOLUME_SHADOW_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b2, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=1, b100)," \
"RootConstants(num32BitConstants=1, b101)," \
"SRV(t0)"

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBSunLight : register(b1)
{
	SunLightData g_sun;
}

// 歩く範囲と歩数。
// ※ CPU 側(RaytracingVolumeShadowPass の VolumeShadowCB)と並びを合わせること
struct VolumeShadowParam
{
	float distance;		// 歩く範囲(ビュー空間の奥行き。シーンの影の距離)
	uint frame;			// 歩く位置のずらしをフレームごとに変える
	uint stepCount;		// 視線を歩く歩数(1歩ごとにレイを1本)
	float pad;
};

cbuffer CBVolumeShadow : register(b2)
{
	VolumeShadowParam g_volume;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_depthTexIndex;
}

Texture2D<float> Get_depthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_depthTexIndex]; return _r; }	// シーンの深度(フル解像度)
#define g_depthTex Get_depthTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float2> Get_outTex() { RWTexture2D<float2> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }	// フォグ用の影(低解像度)
#define g_outTex Get_outTex()

RaytracingAccelerationStructure g_raytracingWorld : register(t0);

// 主光源が見えるか : 1 = 日なた / 0 = 影。
// 当たったかどうかだけ分かればよいので、最初に当たった時点で打ち切る
float TraceSunVisibility(float3 a_origin, float3 a_toSun)
{
	RayDesc _ray;
	_ray.Origin = a_origin;
	_ray.Direction = a_toSun;
	_ray.TMin = 0.001f;
	_ray.TMax = 10000;

	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> _query;
	_query.TraceRayInline(g_raytracingWorld, RAY_FLAG_NONE, 0xFF, _ray);

	// 全部不透明扱いなので、候補を自分で判定する必要は無い(1回で走査が終わる)
	_query.Proceed();

	return (_query.CommittedStatus() == COMMITTED_NOTHING) ? 1.0f : 0.0f;
}

// 画素とフレームで変わる 0..1 のずらし(Interleaved Gradient Noise)
float InterleavedGradientNoise(float2 a_pixel, float a_frame)
{
	a_pixel += 5.588238f * a_frame;
	return frac(52.9829189f * frac(dot(a_pixel, float2(0.06711056f, 0.00583715f))));
}

[RootSignature(RAY_VOLUME_SHADOW_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);
	if (DTid.x >= _width || DTid.y >= _height) return;

	const uint2 _id = DTid.xy;

	// 平行光が無い・歩かない : 遮るものも無い扱い
	if (g_sun.enable == 0 || g_volume.stepCount == 0)
	{
		g_outTex[_id] = float2(1.0f, 0.0f);
		return;
	}

	// この画素の中心が指すフル解像度の画素の深度
	uint _fullWidth, _fullHeight;
	g_depthTex.GetDimensions(_fullWidth, _fullHeight);

	const float2 _uv = (float2(_id) + 0.5f) / float2(_width, _height);
	const uint2 _fullId = min(uint2(_uv * float2(_fullWidth, _fullHeight)), uint2(_fullWidth - 1, _fullHeight - 1));
	const float _depth = g_depthTex.Load(int3(_fullId, 0));

	// 視線 : 空の画素は遠い面の向きへ
	const float3 _start = g_camera.cameraPos.xyz;
	const bool _isSky = (_depth >= 1.0f);
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, _isSky ? 1.0f : _depth, 1.0f);
	float4 _end4 = mul(_clip, g_camera.invViewProj);
	const float3 _end = _end4.xyz / _end4.w;
	const float3 _toEnd = _end - _start;
	const float3 _dir = normalize(_toEnd);
	const float _length = _isSky ? 1e30f : length(_toEnd);

	// 範囲の終わり : ビュー空間の奥行きで distance まで
	const float _viewZPerDistance = mul(float4(_dir, 0.0f), g_camera.view).z;
	if (_viewZPerDistance <= 0.0001f)
	{
		g_outTex[_id] = float2(1.0f, 0.0f);
		return;
	}
	const float _rangeEnd = min(_length, g_volume.distance / _viewZPerDistance);

	const float3 _toSun = normalize(-g_sun.dir);
	const float _jitter = InterleavedGradientNoise(float2(_id), (float) (g_volume.frame % 64));
	const float _stepLength = _rangeEnd / (float) g_volume.stepCount;

	float _visibility = 0.0f;
	for (uint _i = 0; _i < g_volume.stepCount; ++_i)
	{
		const float _t = (_i + _jitter) * _stepLength;
		_visibility += TraceSunVisibility(_start + _dir * _t, _toSun);
	}

	g_outTex[_id] = float2(_visibility / (float) g_volume.stepCount, _rangeEnd);
}
