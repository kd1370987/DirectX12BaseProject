//==========================================================================================
//
// GroundVolumetricFogCS
//
// 地面メッシュから一定の高さ(fogHeight)まで、チリの層を積分する。
//   rgb = フォグの色 / a = フォグの濃さ(0..1)
//
// ・チリの層は常にある。高さは「その画素で見えている地面」の高さから測る
//   (レイ上の各点の真下の地面は画面に映っているとは限らないので、画面空間で近似する。
//    レイが段差や崖の上を通るところでは、層の高さが少しずれる)
// ・衝撃が来ると、波が通り過ぎた内側のチリが払われ、波頭へ寄せられて巻き上がる。
//   時間が経つと払った場所へチリが戻る(式は GroundFieldData.hlsli に共通化してある)
//
//   濃さ = density × 高さの減衰 × ノイズ × (払われずに残った量 + 波頭に寄せられた量)
//
// 震源からの距離は水平(xz)で測る。波の輪は地面の起伏へ真上から投影した形になり、
// 層の高さも地面から測るので、地形に沿って広がる
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
//   2 : SRVの番号(t0-t1) 地面の深度 + シーンの深度(任意。レンダーグラフが張る)
//   3 : UAVの番号(u0)    フォグ(レンダーグラフが張る)
//   4 : SRVの番号(t2)    ノイズテクスチャ(パスが張る)
//   5 : SRVの番号(t3)    衝撃の配列(GraphicsEngine が詰めたもの)
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
"RootConstants(num32BitConstants=1, b103), " \
RS_STATIC_SAMPLER

// 張られていないときの番号(未接続の入力・未設定のノイズ。パス側と合わせる)
#define DESCRIPTOR_INDEX_NONE 0xFFFFFFFF

// レイマーチの歩数の上限。
// 遠い地面ほど歩数が増えるので、ここで頭打ちにして1歩を伸ばす
#define GROUND_FOG_MAX_STEPS 64

// 波頭でチリが巻き上がる高さの上限(fogHeight の何倍まで)。
// レイはこの高さで切るので、上げすぎると歩数が層の外で無駄になる
#define GROUND_FOG_MAX_LIFT 2.0f

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
	uint g_sceneDepthTexIndex;		// 任意 : 未接続なら DESCRIPTOR_INDEX_NONE
}

Texture2D<float> Get_groundDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_groundDepthTexIndex]; return _r; }	// 地面だけの深度
#define g_groundDepthTex Get_groundDepthTex()
Texture2D<float> Get_sceneDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_sceneDepthTexIndex]; return _r; }		// シーン全体の深度(手前の物体でレイを止める)
#define g_sceneDepthTex Get_sceneDepthTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float4> Get_outTex() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }
#define g_outTex Get_outTex()

// ノイズ
// グラフのリソースではないので、パスが自分で番号を渡す。未設定なら DESCRIPTOR_INDEX_NONE
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

// サンプラー
SamplerState g_samp : register(s0);

// 画素のUVと深度からワールド座標を戻す
float3 ReconstructWorldPos(float2 a_uv, float a_depth)
{
	float4 _ndc = float4(a_uv.x * 2.0f - 1.0f, 1.0f - a_uv.y * 2.0f, a_depth, 1.0f);
	float4 _worldPos = mul(_ndc, g_camera.invViewProj);
	return _worldPos.xyz / _worldPos.w;
}

// 水平位置(xz)で、衝撃がチリをどう動かしたか。
//   x = 払われずに残った量(1 = 手つかず。衝撃が重なると掛け合わせで減る)
//   y = 波頭に寄せられた量(衝撃が重なると足し合わせ)
float2 CalcDustAt(float2 a_posXZ)
{
	float _remain = 1.0f;
	float _pile = 0.0f;
	for (uint _i = 0; _i < g_fogData.impulseCount; ++_i)
	{
		GroundImpulse _impulse = g_impulses[_i];
		float _distance = distance(a_posXZ, _impulse.pos.xz);

		_remain *= 1.0f - CalcGroundImpulseSweep(_impulse, _distance);
		_pile += CalcGroundImpulseWave(_impulse, _distance);
	}
	return float2(_remain, _pile);
}

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
	const float4 _noFog = float4(g_fogData.fogColor, 0.0f);

	// 地面が描かれていないピクセルはフォグなし。
	// 層の高さを測る基準が無いうえ、遠平面までマーチすると歩数が跳ね上がる
	float _groundDepth = g_groundDepthTex.Load(int3(_coord, 0));
	if (_groundDepth >= 1.0f)
	{
		g_outTex[_coord] = _noFog;
		return;
	}

	// 画素の中心を指すUV(+0.5 を足さないと半画素ずれる)
	float2 _uv = (float2(_coord) + 0.5f) / float2(_width, _height);

	// 層の高さの基準 : この画素で見えている地面
	float3 _groundPos = ReconstructWorldPos(_uv, _groundDepth);
	float _groundY = _groundPos.y;

	// レイの終点 : 地面より手前に物体があれば、そこで止める。
	// 地面の深度には地面しか描かれていないので、止めないと物体の上にチリが重なる
	float _endDepth = _groundDepth;
	if (g_sceneDepthTexIndex != DESCRIPTOR_INDEX_NONE)
	{
		_endDepth = min(_endDepth, g_sceneDepthTex.Load(int3(_coord, 0)));
	}

	float3 _rayStart = g_camera.cameraPos.xyz;
	float3 _rayEnd = (_endDepth < _groundDepth) ? ReconstructWorldPos(_uv, _endDepth) : _groundPos;

	// レイをチリの層(地面から巻き上がりの上限まで)だけに切り詰める。
	// 層の外は濃さが 0 なので、歩数をすべて層の中へ使う
	float _fogHeight = max(g_fogData.fogHeight, 0.0001f);
	float _layerTop = _groundY + _fogHeight * GROUND_FOG_MAX_LIFT;
	if (_rayStart.y > _layerTop && _rayEnd.y > _layerTop)
	{
		g_outTex[_coord] = _noFog;
		return;
	}

	float _tStart = 0.0f;
	float _tEnd = 1.0f;
	if (_rayStart.y > _layerTop) _tStart = (_rayStart.y - _layerTop) / (_rayStart.y - _rayEnd.y);	// 上から層へ入る
	if (_rayEnd.y > _layerTop) _tEnd = (_layerTop - _rayStart.y) / (_rayEnd.y - _rayStart.y);		// 層から上へ抜ける

	float _distance = length(_rayEnd - _rayStart) * (_tEnd - _tStart);

	// 歩数 : 上限で頭打ちにしたぶんは1歩を伸ばして、レイの最後まで届かせる
	uint _steps = (uint) ceil(_distance / max(g_fogData.stepSize, 0.01f));
	_steps = min(_steps, (uint) GROUND_FOG_MAX_STEPS);
	float _stepLength = _distance / max((float) _steps, 1.0f);

	const bool _isUseNoise = (g_noiseTexIndex != DESCRIPTOR_INDEX_NONE);

	// フォグ積分
	float _fogAmount = 0.0f;

	// ステップごとに計算
	for (uint _i = 0; _i < _steps; ++_i)
	{
		float _t = lerp(_tStart, _tEnd, (_i + 0.5f) / _steps);

		float3 _samplePos = lerp(_rayStart, _rayEnd, _t);

		// 衝撃で払われた量と、波頭に寄せられた量
		float2 _dust = CalcDustAt(_samplePos.xz);

		// 地面からの高さ。波頭では寄せられたぶんだけ層が高くなる(巻き上がり)
		float _heightFromGround = _samplePos.y - _groundY;
		float _layerHeight = _fogHeight * min(1.0f + _dust.y, GROUND_FOG_MAX_LIFT);

		// 地面付近だけチリを置く
		float _heightMask = 1.0f - saturate(_heightFromGround / _layerHeight);
		if (_heightMask <= 0.0f) continue;

		// ノイズ : 未設定なら一様(1)
		float _noise = 1.0f;
		if (_isUseNoise)
		{
			_noise = g_noiseTex.SampleLevel(g_samp, _samplePos.xz * g_fogData.noiseScale + g_fogData.time * 0.01f, 0).r;
		}

		// 密度 : 払われずに残ったチリ + 波頭に寄せられたチリ
		float _density = g_fogData.density * _heightMask * _noise * (_dust.x + _dust.y);

		_fogAmount += _density * _stepLength;
	}

	_fogAmount = 1.0f - exp(-_fogAmount);

	g_outTex[_coord] = float4(g_fogData.fogColor, _fogAmount);
}
