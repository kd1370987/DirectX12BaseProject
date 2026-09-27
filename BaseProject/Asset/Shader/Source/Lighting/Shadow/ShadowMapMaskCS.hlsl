//==========================================================================================
//
// ShadowMapMaskCS
//
// ShadowMapPass が光源から描いた深度(シャドウマップ)と比べて、主光源の影マスクを作る。
// 出力はレイトレの影(RaytracingShadowPass)と同じ形で、1 = ひなた / 0 = 影。
//
// シャドウマップはカスケードごとに 2x2 のタイルへ分けた1枚のアトラス。
//   カスケード i のタイル : 左上から (i % 2, i / 2)
// どのカスケードを引くかは、ピクセルのビュー空間の奥行きで決める。
// 境目は次のカスケードへ混ぜ、最後のカスケードの先は影なしへ混ぜる(影の届く端がぼける)。
//
// 行列・区切り・バイアスは LightManager::BuildShadowCascades() が毎フレーム組む
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"
#include "../../../Common/Math/CalcNormal.hlsli"
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/ShadowMapData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            シャドウマップ(カスケードの行列・区切り・バイアス)
//   2 : SRVの番号(t0-t2) 深度 / 法線 / シャドウマップ(レンダーグラフが張る)
//   3 : UAVの番号(u0)    影マスク(レンダーグラフが張る)
//
// サンプラーは比較サンプラー(PCF用)。
// 比べた結果をバイリニアで混ぜて返すので、1回引くだけで縁が2x2テクセルぶんなめらかになる
//==========================================================================================
#define SHADOW_MASK_ROOT_SIG \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=3, b100)," \
"RootConstants(num32BitConstants=1, b101)," \
"StaticSampler(s0, " \
"    filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, " \
"    addressU = TEXTURE_ADDRESS_CLAMP, " \
"    addressV = TEXTURE_ADDRESS_CLAMP, " \
"    addressW = TEXTURE_ADDRESS_CLAMP, " \
"    comparisonFunc = COMPARISON_LESS_EQUAL)"

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBShadowMap : register(b1)
{
	ShadowMapData g_shadow;
}

// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_depthTexIndex;
	uint g_normalTexIndex;
	uint g_shadowMapIndex;
}

Texture2D Get_depthTex() { Texture2D _r = ResourceDescriptorHeap[g_depthTexIndex]; return _r; }
#define g_depthTex Get_depthTex()
Texture2D Get_normalTex() { Texture2D _r = ResourceDescriptorHeap[g_normalTexIndex]; return _r; }
#define g_normalTex Get_normalTex()
Texture2D<float> Get_shadowMap() { Texture2D<float> _r = ResourceDescriptorHeap[g_shadowMapIndex]; return _r; }
#define g_shadowMap Get_shadowMap()

// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outputIndex;
}

RWTexture2D<float4> Get_output() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outputIndex]; return _r; }	// 影マスク
#define g_output Get_output()

SamplerComparisonState g_shadowSamp : register(s0);

// アトラスの1辺あたりのタイル数
static const uint kAtlasTiles = 2;

//------------------------------------------------------------------------------------------
// カスケード1枚ぶんの遮蔽 : 1 = ひなた / 0 = 影
//------------------------------------------------------------------------------------------
float SampleCascade(uint a_cascade, float3 a_worldPos, float3 a_normal, float a_tileSize)
{
	// 1テクセルがワールドでどれだけの大きさか
	float _texelWorld = (g_shadow.cascadeRadius[a_cascade] * 2.0f) / a_tileSize;

	// 法線方向へずらしてから光源の空間へ移す。
	// テクセルの大きさに比例させるので、粗いカスケードほど大きくずれる(アクネも粗くなるため)
	float3 _pos = a_worldPos + a_normal * (_texelWorld * g_shadow.normalBias);
	float4 _lightClip = mul(float4(_pos, 1.0f), g_shadow.lightViewProj[a_cascade]);
	float3 _ndc = _lightClip.xyz / _lightClip.w;

	// 箱の奥より先は描かれていない(球で囲んでいるので本来は来ない)
	if (_ndc.z >= 1.0f) return 1.0f;

	float2 _uv = float2(_ndc.x * 0.5f + 0.5f, 0.5f - _ndc.y * 0.5f);

	// 比べる深度。バイアスはワールドの長さで持っているので、箱の奥行きで割って射影後の値にする
	// (正射影なので深度は奥行きに比例する)
	float _compareDepth = _ndc.z - g_shadow.depthBias / g_shadow.cascadeDepthRange[a_cascade];

	// タイルの中だけを引く。
	// 端まで行くとバイリニアが隣のタイル(別のカスケード)を拾うので、1テクセル内側で止める
	float _texel = 1.0f / a_tileSize;
	float2 _minUV = float2(_texel, _texel);
	float2 _maxUV = float2(1.0f - _texel, 1.0f - _texel);
	float2 _tileOrigin = float2(a_cascade % kAtlasTiles, a_cascade / kAtlasTiles) / kAtlasTiles;

	// PCF : 3x3 を比較サンプラーのバイリニアで引く
	float _sum = 0.0f;
	[unroll]
	for (int _y = -1; _y <= 1; ++_y)
	{
		[unroll]
		for (int _x = -1; _x <= 1; ++_x)
		{
			float2 _tileUV = clamp(_uv + float2(_x, _y) * (g_shadow.softness * _texel), _minUV, _maxUV);
			float2 _atlasUV = _tileOrigin + _tileUV / kAtlasTiles;
			_sum += g_shadowMap.SampleCmpLevelZero(g_shadowSamp, _atlasUV, _compareDepth);
		}
	}
	return _sum / 9.0f;
}

[RootSignature(SHADOW_MASK_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	uint _width, _height;
	g_output.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height)
		return;

	int2 _coord = int2(DTid.xy);

	// シャドウマップを使わないフレーム(平行光が無い・レイトレで求めている)は影なしで埋める
	if (g_shadow.cascadeCount == 0)
	{
		g_output[_coord] = float4(1, 1, 1, 1);
		return;
	}

	// 何も描かれていないピクセル(空)は影なし
	float _depth = g_depthTex.Load(int3(_coord, 0)).r;
	if (_depth >= 1.0f)
	{
		g_output[_coord] = float4(1, 1, 1, 1);
		return;
	}

	// 光が当たらない裏面は影(レイトレの影と同じ扱い)。
	// 明暗の境目でシャドウマップと比べるとアクネが出やすいので、比べずに決める
	float3 _normal = DecsodeNormal(g_normalTex.Load(int3(_coord, 0)).rg);
	float3 _toLight = -g_shadow.lightDir;
	if (dot(_normal, _toLight) <= 0.0f)
	{
		g_output[_coord] = float4(0, 0, 0, 1);
		return;
	}

	// 3D空間での位置を復元
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, _depth, 1.0f);
	float4 _worldPos4 = mul(_clip, g_camera.invViewProj);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;

	// カスケードの区切りはビュー空間の奥行きで持っている
	float _viewZ = mul(float4(_worldPos, 1.0f), g_camera.view).z;

	// どのカスケードに入るか : 影の届く端より奥はどれにも入らない
	uint _cascade = g_shadow.cascadeCount;
	for (uint _i = 0; _i < g_shadow.cascadeCount; ++_i)
	{
		if (_viewZ <= g_shadow.cascadeFar[_i])
		{
			_cascade = _i;
			break;
		}
	}

	if (_cascade >= g_shadow.cascadeCount)
	{
		g_output[_coord] = float4(1, 1, 1, 1);
		return;
	}

	// タイル1枚の解像度
	uint _mapWidth, _mapHeight;
	g_shadowMap.GetDimensions(_mapWidth, _mapHeight);
	float _tileSize = (float)_mapWidth / kAtlasTiles;

	float _shadow = SampleCascade(_cascade, _worldPos, _normal, _tileSize);

	// 境目は次のカスケードへ混ぜる。最後のカスケードは影なしへ混ぜる
	float _blendStart = g_shadow.cascadeBlendStart[_cascade];
	if (_viewZ > _blendStart)
	{
		float _blendWidth = max(g_shadow.cascadeFar[_cascade] - _blendStart, 1e-4f);
		float _t = saturate((_viewZ - _blendStart) / _blendWidth);

		float _next = 1.0f;
		if (_cascade + 1 < g_shadow.cascadeCount)
		{
			_next = SampleCascade(_cascade + 1, _worldPos, _normal, _tileSize);
		}
		_shadow = lerp(_shadow, _next, _t);
	}

	g_output[_coord] = float4(_shadow, _shadow, _shadow, 1.0f);
}
