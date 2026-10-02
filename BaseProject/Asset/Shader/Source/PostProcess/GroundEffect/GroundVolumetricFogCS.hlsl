//==========================================================================================
//
// GroundVolumetricFogCS
//
// カメラから地面までのレイをマーチして、地面付近に立つフォグを積分する。
//   rgb = フォグの色 / a = フォグの濃さ(0..1)
//
// 濃さ = 衝撃の波 × 高さの減衰 × ノイズ × density
//
// 衝撃の波は、レイの1歩ごとに衝撃の配列から直接求める(式は GroundFieldCS と共通)。
// 震源からの距離は水平(xz)で測るので、地面の波紋の上に壁のように立ち上がる
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/GroundFieldData.hlsli"
#include "../../../Common/RootParameters/GroundFogData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            フォグの調整値
//   2 : SRVの番号(t0)    地面の深度(レンダーグラフが張る)
//   3 : UAVの番号(u0)    フォグ(レンダーグラフが張る)
//   4 : SRVの番号(t1)    ノイズテクスチャ(パスが張る)
//   5 : SRVの番号(t2)    衝撃の配列(GraphicsEngine が詰めたもの)
//
// 追加は必ず末尾へ足すこと。間に挟むと既存の番号が全部ずれる
//==========================================================================================
#define GROUND_VOLUMETRIC_FOG_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=1, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
"RootConstants(num32BitConstants=1, b102), " \
"RootConstants(num32BitConstants=1, b103), " \
RS_STATIC_SAMPLER

// ノイズテクスチャが張られていないときの番号(パス側と合わせる)
#define NOISE_INDEX_NONE 0xFFFFFFFF

// レイマーチの歩数の上限。
// 遠い地面ほど歩数が増えるので、ここで頭打ちにして1歩を伸ばす
#define GROUND_FOG_MAX_STEPS 64

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBGroundFog : register(b1)
{
	GroundFogData g_fogData;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_groundDepthTexIndex;
}

Texture2D<float> Get_groundDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_groundDepthTexIndex]; return _r; }	// 地面の深度
#define g_groundDepthTex Get_groundDepthTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float4> Get_outTex() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }
#define g_outTex Get_outTex()

// ノイズ
// グラフのリソースではないので、パスが自分で番号を渡す。未設定なら NOISE_INDEX_NONE
cbuffer PassDescriptorIndex2 : register(b102)
{
	uint g_noiseTexIndex;
}

Texture2D<float4> Get_noiseTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_noiseTexIndex]; return _r; }	// ノイズ(r)
#define g_noiseTex Get_noiseTex()

// 衝撃の配列 : GraphicsEngine が毎フレーム詰め直したもの。
// 要素数は g_fogData.impulseCount
cbuffer PassDescriptorIndex3 : register(b103)
{
	uint g_impulsesIndex;
}

StructuredBuffer<GroundImpulse> Get_impulses() { StructuredBuffer<GroundImpulse> _r = ResourceDescriptorHeap[g_impulsesIndex]; return _r; }
#define g_impulses Get_impulses()

// 水平位置(xz)での衝撃の波の合計
float CalcFieldAt(float2 a_posXZ)
{
	float _field = 0.0f;
	for (uint _i = 0; _i < g_fogData.impulseCount; ++_i)
	{
		GroundImpulse _impulse = g_impulses[_i];
		_field += CalcGroundImpulseWave(_impulse, distance(a_posXZ, _impulse.pos.xz));
	}
	return _field;
}

// サンプラー
SamplerState g_samp : register(s0);

[RootSignature(GROUND_VOLUMETRIC_FOG_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _coord = int2(DTid.xy);

	// 地面が描かれていないピクセルと、衝撃が1つも無いフレームはフォグなし。
	// 遠平面までマーチすると歩数が跳ね上がるので、ここで抜ける
	float _depth = g_groundDepthTex.Load(int3(_coord, 0));
	if (_depth >= 1.0f || g_fogData.impulseCount == 0)
	{
		g_outTex[_coord] = float4(g_fogData.fogColor, 0.0f);
		return;
	}

	// 深度からワールド座標を復元
	// 画素の中心を指すUV(+0.5 を足さないと半画素ずれる)
	float2 _uv = (float2(_coord) + 0.5f) / float2(_width, _height);

	float4 _ndc;
	_ndc.xy = _uv * 2.0f - 1.0f;
	_ndc.y *= -1.0f;
	_ndc.z = _depth;
	_ndc.w = 1.0f;

	float4 _worldPos = mul(_ndc, g_camera.invViewProj);
	_worldPos.xyz /= _worldPos.w;

	// カメラから地面までのレイ
	float3 _rayStart = g_camera.cameraPos.xyz;
	float3 _rayEnd = _worldPos.xyz;

	// レイをフォグの層(高さ fogHeight より下)だけに切り詰める。
	// 層の外は濃さが 0 なので、歩数をすべて層の中へ使う
	float _fogHeight = max(g_fogData.fogHeight, 0.0001f);
	if (_rayStart.y > _fogHeight && _rayEnd.y > _fogHeight)
	{
		g_outTex[_coord] = float4(g_fogData.fogColor, 0.0f);
		return;
	}

	float _tStart = 0.0f;
	float _tEnd = 1.0f;
	if (_rayStart.y > _fogHeight) _tStart = (_rayStart.y - _fogHeight) / (_rayStart.y - _rayEnd.y);	// 上から層へ入る
	if (_rayEnd.y > _fogHeight) _tEnd = (_fogHeight - _rayStart.y) / (_rayEnd.y - _rayStart.y);		// 層から上へ抜ける

	float _distance = length(_rayEnd - _rayStart) * (_tEnd - _tStart);

	// 歩数 : 上限で頭打ちにしたぶんは1歩を伸ばして、レイの最後まで届かせる
	uint _steps = (uint) ceil(_distance / max(g_fogData.stepSize, 0.01f));
	_steps = min(_steps, (uint) GROUND_FOG_MAX_STEPS);
	float _stepLength = _distance / max((float) _steps, 1.0f);

	const bool _isUseNoise = (g_noiseTexIndex != NOISE_INDEX_NONE);

	// フォグ積分
	float _fogAmount = 0.0f;

	// ステップごとに計算
	for (uint _i = 0; _i < _steps; ++_i)
	{
		float _t = lerp(_tStart, _tEnd, (_i + 0.5f) / _steps);

		float3 _samplePos = lerp(_rayStart, _rayEnd, _t);

		// 地面からの高さ
		float _sampleHeight = _samplePos.y;

		// 地面付近だけフォグを発生させる
		float _heightMask = 1.0f - saturate(_sampleHeight / _fogHeight);

		// 衝撃の波 : サンプル位置の真下で衝撃の配列から求める
		float _field = CalcFieldAt(_samplePos.xz);
		if (_field <= 0.0f) continue;

		// ノイズ : 未設定なら一様(1)
		float _noise = 1.0f;
		if (_isUseNoise)
		{
			_noise = g_noiseTex.SampleLevel(g_samp, _samplePos.xz * g_fogData.noiseScale + g_fogData.time * 0.01f, 0).r;
		}

		// 密度
		float _density = _field * _heightMask * _noise * g_fogData.density;

		_fogAmount += _density * _stepLength;
	}

	_fogAmount = 1.0f - exp(-_fogAmount);

	g_outTex[_coord] = float4(g_fogData.fogColor, _fogAmount);
}
