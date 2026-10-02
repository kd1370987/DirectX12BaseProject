//==========================================================================================
//
// GroundVolumetricFogCS
//
// カメラから地面までのレイをマーチして、地面付近に立つフォグを積分する。
//   rgb = フォグの色 / a = フォグの濃さ(0..1)
//
// 濃さ = グラウンドフィールド(衝撃) × 高さの減衰 × ノイズ × density
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/GroundFogData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            フォグの調整値
//   2 : SRVの番号(t0-t1) 地面の深度 + グラウンドフィールド(レンダーグラフが張る)
//   3 : UAVの番号(u0)    フォグ(レンダーグラフが張る)
//   4 : SRVの番号(t2)    ノイズテクスチャ(パスが張る)
//
// 追加は必ず末尾へ足すこと。間に挟むと既存の番号が全部ずれる
//==========================================================================================
#define GROUND_VOLUMETRIC_FOG_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=2, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
"RootConstants(num32BitConstants=1, b102), " \
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
	uint g_groundFieldTexIndex;
}

Texture2D<float> Get_groundDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_groundDepthTexIndex]; return _r; }		// 地面の深度
#define g_groundDepthTex Get_groundDepthTex()
Texture2D<float4> Get_groundFieldTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_groundFieldTexIndex]; return _r; }	// 地面の衝撃計算結果(r)
#define g_groundFieldTex Get_groundFieldTex()

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

	// 地面が描かれていないピクセルはフォグなし。
	// 遠平面までマーチすると歩数が跳ね上がるので、ここで抜ける
	float _depth = g_groundDepthTex.Load(int3(_coord, 0));
	if (_depth >= 1.0f)
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

	float _distance = length(_rayEnd - _rayStart);

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
		float _t = (_i + 0.5f) / _steps;

		float3 _samplePos = lerp(_rayStart, _rayEnd, _t);

		// 地面からの高さ
		float _sampleHeight = _samplePos.y;

		// 地面付近だけフォグを発生させる
		float _heightMask = 1.0f - saturate(_sampleHeight / max(g_fogData.fogHeight, 0.0001f));

		// GroundField をサンプリング
		// グラウンドフィールドは画面空間なので、サンプル位置を画面へ投影して引く
		float4 _sampleClip = mul(float4(_samplePos, 1.0f), g_camera.viewProj);
		float2 _fieldUV = _sampleClip.xy / _sampleClip.w * float2(0.5f, -0.5f) + 0.5f;

		float _field = g_groundFieldTex.SampleLevel(g_samp, _fieldUV, 0).r;

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
