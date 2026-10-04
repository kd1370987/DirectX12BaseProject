//==========================================================================================
//
// SceneVolumetricFogCS
//
// カメラから見えている面(空なら maxDistance 先)までのレイに沿って、
// 2つの媒質を積分する。
//   ・シーンのフォグ   : シーン全体に一様に漂う(SceneFogData)
//   ・グラウンドダスト : 地面から一定の高さまで漂うチリ(GroundDustData)
// 出力 : rgb = フォグの色(2つの媒質の色を濃さで混ぜたもの) / a = フォグの濃さ(0..1)
//
// シーンのフォグは一様なので、ダストの層の外は式で一度に求める。
// レイマーチするのはダストの層の中だけ(歩数を層の中へ集めるため)。
//
// ・ダストの高さは、レイの終点の真下の地面から測る
//     地面の画素 : 見えている地面そのもの
//     物体の画素 : 物体の点の真下の地面を、地面だけの深度から探して使う
//   (レイ上の各点の真下の地面は画面に映っているとは限らないので、画面空間で近似する。
//    レイが段差や崖の上を通るところでは、層の高さが少しずれる)
// ・衝撃でチリが払われる・波頭へ寄せられる量は、GroundFieldPass が書いた
//   真上からのテクスチャを引く。波頭では寄せられたぶんだけ層が高くなる(巻き上がり)
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/GroundFieldData.hlsli"
#include "../../../Common/RootParameters/SceneFogData.hlsli"
#include "../../../Common/RootParameters/GroundDustData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            シーンのフォグの調整値
//   2 : CBV(b2)            グラウンドダストの調整値
//   3 : SRVの番号(t0-t2) シーンの深度 + 地面の深度(任意) + グラウンドフィールド(任意)
//                          (レンダーグラフが張る)
//   4 : UAVの番号(u0)    フォグ(レンダーグラフが張る)
//   5 : SRVの番号(t3)    ノイズテクスチャ(パスが張る)
//
// 追加は必ず末尾へ足すこと。間に挟むと既存の番号が全部ずれる
//==========================================================================================
#define SCENE_VOLUMETRIC_FOG_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b2, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=3, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
"RootConstants(num32BitConstants=1, b102), " \
RS_STATIC_SAMPLER

// 張られていないときの番号(未接続の入力・未設定のノイズ。パス側と合わせる)
#define DESCRIPTOR_INDEX_NONE 0xFFFFFFFF

// ダストの層の中をレイマーチする歩数の上限。
// 層の中が長いほど歩数が増えるので、ここで頭打ちにして1歩を伸ばす
#define GROUND_DUST_MAX_STEPS 64

// 波頭でチリが巻き上がる高さの上限(height の何倍まで)。
// レイはこの高さで切るので、上げすぎると歩数が層の外で無駄になる
#define GROUND_DUST_MAX_LIFT 2.0f

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBSceneFog : register(b1)
{
	SceneFogData g_sceneFog;
}

cbuffer CBGroundDust : register(b2)
{
	GroundDustData g_groundDust;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_sceneDepthTexIndex;
	uint g_groundDepthTexIndex;		// 任意 : 未接続なら DESCRIPTOR_INDEX_NONE(ダストは出ない)
	uint g_groundFieldTexIndex;		// 任意 : 未接続なら DESCRIPTOR_INDEX_NONE(衝撃でチリが動かない)
}

Texture2D<float> Get_sceneDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_sceneDepthTexIndex]; return _r; }		// シーン全体の深度(レイの終点)
#define g_sceneDepthTex Get_sceneDepthTex()
Texture2D<float> Get_groundDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_groundDepthTexIndex]; return _r; }	// 地面だけの深度(ダストの高さの基準)
#define g_groundDepthTex Get_groundDepthTex()
Texture2D<float2> Get_groundFieldTex() { Texture2D<float2> _r = ResourceDescriptorHeap[g_groundFieldTexIndex]; return _r; }	// グラウンドフィールド(r = 残ったチリ / g = 寄せられたチリ)
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
// グラフのリソースではないので、パスが自分で番号を渡す。未設定なら DESCRIPTOR_INDEX_NONE
cbuffer PassDescriptorIndex2 : register(b102)
{
	uint g_noiseTexIndex;
}

Texture2D<float4> Get_noiseTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_noiseTexIndex]; return _r; }	// ノイズ(r)
#define g_noiseTex Get_noiseTex()

// サンプラー
SamplerState g_samp : register(s0);

// 画素のUVと深度からワールド座標を戻す
float3 ReconstructWorldPos(float2 a_uv, float a_depth)
{
	float4 _ndc = float4(a_uv.x * 2.0f - 1.0f, 1.0f - a_uv.y * 2.0f, a_depth, 1.0f);
	float4 _worldPos = mul(_ndc, g_camera.invViewProj);
	return _worldPos.xyz / _worldPos.w;
}

// 物体の点の真下にある地面の高さを、地面だけの深度から探す。
//
// 物体の画素の奥に見えている地面は、物体とは関係のない遠くの地面(または空)なので、
// ダストの層の基準にすると物体の上に「奥が地面か空か」の境目がそのまま出てしまう。
//
// 点の真下の候補(x, 推定高さ, z)を画面へ投影し、そこに見えている地面を復元して
// 推定高さを置き換える、を繰り返す。平らな地面なら1回、起伏があっても数回で収まる。
// 地面だけの深度には物体が描かれていないので、物体の足元に隠れた地面も引ける。
// 画面の外へ出た・その先が空だったときは、それまでに求まった値で打ち切る
//   戻り値 : 1回でも地面を引けたか
bool EstimateGroundYBelow(float3 a_pos, float a_initY, float2 a_size, out float a_outY)
{
	float _groundY = a_initY;
	bool _isFound = false;

	[unroll]
	for (uint _i = 0; _i < 4; ++_i)
	{
		float4 _clip = mul(float4(a_pos.x, _groundY, a_pos.z, 1.0f), g_camera.viewProj);
		if (_clip.w <= 0.0f) break;		// カメラの後ろ

		float2 _uv = float2(_clip.x / _clip.w * 0.5f + 0.5f, 0.5f - _clip.y / _clip.w * 0.5f);
		if (any(_uv < 0.0f) || any(_uv > 1.0f)) break;	// 画面の外

		int2 _pixel = min(int2(_uv * a_size), int2(a_size) - 1);
		float _depth = g_groundDepthTex.Load(int3(_pixel, 0));
		if (_depth >= 1.0f) break;		// その先に地面が無い

		float _newY = ReconstructWorldPos((float2(_pixel) + 0.5f) / a_size, _depth).y;
		const bool _isConverged = _isFound && abs(_newY - _groundY) < 0.01f;

		_groundY = _newY;
		_isFound = true;
		if (_isConverged) break;
	}

	a_outY = _groundY;
	return _isFound;
}

// 水平位置(xz)のグラウンドフィールドを引く。
//   x = 払われずに残ったチリの量 / y = 波頭に寄せられたチリの量
// 未接続・範囲の外は「衝撃なし」(1, 0)
float2 SampleGroundField(float2 a_posXZ, float2 a_center, float a_halfTexel)
{
	if (g_groundFieldTexIndex == DESCRIPTOR_INDEX_NONE) return float2(1.0f, 0.0f);

	float2 _uv = GroundFieldWorldToUV(a_posXZ, a_center);
	if (any(_uv < 0.0f) || any(_uv > 1.0f)) return float2(1.0f, 0.0f);

	// サンプラーは WRAP(ノイズ用)なので、縁で反対側を拾わないよう半テクセル内側へ寄せる
	_uv = clamp(_uv, a_halfTexel, 1.0f - a_halfTexel);
	return g_groundFieldTex.SampleLevel(g_samp, _uv, 0);
}

// シーンのフォグだけが漂う区間を、式で一度に積分する(一様なので歩かなくてよい)
void IntegrateSceneFog(float a_length, inout float3 a_inoutColor, inout float a_inoutTransmittance)
{
	if (a_length <= 0.0f) return;

	const float _transmittance = exp(-g_sceneFog.density * a_length);
	a_inoutColor += a_inoutTransmittance * g_sceneFog.fogColor * (1.0f - _transmittance);
	a_inoutTransmittance *= _transmittance;
}

[RootSignature(SCENE_VOLUMETRIC_FOG_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _coord = int2(DTid.xy);

	const float2 _size = float2(_width, _height);	// 出力画像サイズ
	float2 _uv = (float2(_coord) + 0.5f) / _size;	// 画素の中心を指すUV

	//------------------------------------------------------------------------------
	// レイ : カメラから見えている面まで。空なら maxDistance 先まで
	//------------------------------------------------------------------------------
	const float _sceneDepth = g_sceneDepthTex.Load(int3(_coord, 0));
	const float3 _rayStart = g_camera.cameraPos.xyz;
	float3 _rayEnd;
	if (_sceneDepth < 1.0f)
	{
		_rayEnd = ReconstructWorldPos(_uv, _sceneDepth);
	}
	else
	{
		const float3 _farWorld = ReconstructWorldPos(_uv, 1.0f);
		_rayEnd = _rayStart + normalize(_farWorld - _rayStart) * g_sceneFog.maxDistance;
	}

	const float3 _rayVector = _rayEnd - _rayStart;
	const float _rayLength = length(_rayVector);

	// レイが極端に短ければ切る
	if (_rayLength <= 0.0001f)
	{
		g_outTex[_coord] = float4(g_sceneFog.fogColor, 0.0f);
		return;
	}
	const float3 _rayDir = _rayVector / _rayLength;

	//------------------------------------------------------------------------------
	// ダストの高さの基準になる地面
	//
	// 空の画素は基準が無いのでダストを置かない(シーンのフォグだけ)
	//------------------------------------------------------------------------------
	float _groundY = 0.0f;
	bool _hasGround = false;

	const bool _isUseDust =
		(g_groundDust.density > 0.0f) && (g_groundDepthTexIndex != DESCRIPTOR_INDEX_NONE);

	if (_isUseDust && _sceneDepth < 1.0f)
	{
		const float _groundDepth = g_groundDepthTex.Load(int3(_coord, 0));
		const bool _hasGroundBehind = (_groundDepth < 1.0f);

		if (_hasGroundBehind && _sceneDepth >= _groundDepth)
		{
			// この画素は地面そのもの
			_groundY = ReconstructWorldPos(_uv, _groundDepth).y;
			_hasGround = true;
		}
		else
		{
			// 手前に物体がある -> その物体の真下の地面を探す
			const float _initY = _hasGroundBehind ? ReconstructWorldPos(_uv, _groundDepth).y : _rayEnd.y;
			_hasGround = EstimateGroundYBelow(_rayEnd, _initY, _size, _groundY);

			// 真下の地面が画面から引けない : 奥の地面で代用する(それも無ければダストなし)
			if (!_hasGround && _hasGroundBehind)
			{
				_groundY = _initY;
				_hasGround = true;
			}
		}
	}

	//------------------------------------------------------------------------------
	// ダストの層(地面から巻き上がりの上限まで)に入っている区間 [_dustStart, _dustEnd]
	// 層に入らなければ、区間は空(全体がシーンのフォグだけ)
	//------------------------------------------------------------------------------
	float _dustStart = _rayLength;
	float _dustEnd = _rayLength;
	if (_hasGround)
	{
		const float _layerTop = _groundY + max(g_groundDust.height, 0.0001f) * GROUND_DUST_MAX_LIFT;
		if (_rayStart.y <= _layerTop || _rayEnd.y <= _layerTop)
		{
			_dustStart = 0.0f;
			if (_rayStart.y > _layerTop) _dustStart = _rayLength * (_rayStart.y - _layerTop) / (_rayStart.y - _rayEnd.y);	// 上から層へ入る
			if (_rayEnd.y > _layerTop) _dustEnd = _rayLength * (_layerTop - _rayStart.y) / (_rayEnd.y - _rayStart.y);		// 層から上へ抜ける
		}
	}

	//------------------------------------------------------------------------------
	// 積分
	//   色   : 手前から届く光の和(透過率を掛けながら足す)
	//   透過 : ここまでで残っている割合
	//------------------------------------------------------------------------------
	float3 _color = 0.0f;
	float _transmittance = 1.0f;

	// 層の手前 : シーンのフォグだけ
	IntegrateSceneFog(_dustStart, _color, _transmittance);

	// 層の中 : シーンのフォグ + ダストを歩いて積分する
	const float _dustLength = _dustEnd - _dustStart;
	if (_dustLength > 0.0f)
	{
		uint _steps = (uint) ceil(_dustLength / max(g_groundDust.stepSize, 0.01f));
		_steps = min(_steps, (uint) GROUND_DUST_MAX_STEPS);
		const float _stepLength = _dustLength / max((float) _steps, 1.0f);

		uint _fieldWidth = 1, _fieldHeight = 1;
		if (g_groundFieldTexIndex != DESCRIPTOR_INDEX_NONE)
		{
			g_groundFieldTex.GetDimensions(_fieldWidth, _fieldHeight);
		}
		const float2 _fieldCenter = CalcGroundFieldCenter(_rayStart, (float) _fieldWidth);
		const float _fieldHalfTexel = 0.5f / (float) _fieldWidth;

		const bool _isUseNoise = (g_noiseTexIndex != DESCRIPTOR_INDEX_NONE);
		const float _dustHeight = max(g_groundDust.height, 0.0001f);

		for (uint _i = 0; _i < _steps; ++_i)
		{
			const float3 _samplePos = _rayStart + _rayDir * (_dustStart + (_i + 0.5f) * _stepLength);

			// 衝撃で払われた量と、波頭に寄せられた量
			const float2 _field = SampleGroundField(_samplePos.xz, _fieldCenter, _fieldHalfTexel);

			// 地面からの高さ。波頭では寄せられたぶんだけ層が高くなる(巻き上がり)
			const float _heightFromGround = _samplePos.y - _groundY;
			const float _layerHeight = _dustHeight * min(1.0f + _field.y, GROUND_DUST_MAX_LIFT);
			const float _heightMask = 1.0f - saturate(_heightFromGround / _layerHeight);

			float _dustDensity = 0.0f;
			if (_heightMask > 0.0f)
			{
				// ノイズ : 未設定なら一様(1)
				float _noise = 1.0f;
				if (_isUseNoise)
				{
					_noise = g_noiseTex.SampleLevel(g_samp, _samplePos.xz * g_groundDust.noiseScale + g_groundDust.time * 0.01f, 0).r;
				}

				// 払われずに残ったチリ + 波頭に寄せられたチリ
				_dustDensity = g_groundDust.density * _heightMask * _noise * (_field.x + _field.y);
			}

			// 2つの媒質を合わせた1歩ぶん(Beer-Lambert)。色は濃さの割合で混ぜる
			const float _density = g_sceneFog.density + _dustDensity;
			if (_density <= 0.0f) continue;

			const float _stepTransmittance = exp(-_density * _stepLength);
			const float3 _mediaColor =
				(g_sceneFog.density * g_sceneFog.fogColor + _dustDensity * g_groundDust.dustColor) / _density;

			_color += _transmittance * _mediaColor * (1.0f - _stepTransmittance);
			_transmittance *= _stepTransmittance;
		}
	}

	// 層の奥 : シーンのフォグだけ
	IntegrateSceneFog(_rayLength - _dustEnd, _color, _transmittance);

	//------------------------------------------------------------------------------
	// 出力 : 合成側は lerp(元の色, rgb, a) で重ねるので、色は濃さで割り戻しておく
	//------------------------------------------------------------------------------
	const float _fogAmount = 1.0f - _transmittance;
	const float3 _fogColor = (_fogAmount > 0.00001f) ? _color / _fogAmount : g_sceneFog.fogColor;

	g_outTex[_coord] = float4(_fogColor, _fogAmount);
}
