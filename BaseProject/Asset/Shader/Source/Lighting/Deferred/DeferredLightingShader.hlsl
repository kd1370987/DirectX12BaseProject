#include "../../../Common/Math/CalcNormal.hlsli"
#include "../../../Common/Math/CalcLighting.hlsli"
#include "../../../Common/RootSignatureLayout.hlsli"

// ルートパラメーターの構造体
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/AmbientData.hlsli"
#include "../../../Common/RootParameters/LightingOptionData.hlsli"
#include "../../../Common/RootParameters/LightData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b10)           環境光・フォグ
//   2 : SRVテーブル(t0-t6) GBuffer + 影マスク + GI
//   3 : UAVテーブル(u0)    出力カラー
//   4 : CBV(b11)           ライティング調整値
//   5 : SRVテーブル(t7-t8) ポイントライト配列 + 平行光配列
//   6 : CBV(b12)           ライト数
//
// 追加は必ず末尾へ足すこと。間に挟むと既存の番号が全部ずれる
#define DEFERRED_ROOT_SIG \
"RootFlags(0)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b10, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=7)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
"CBV(b11, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t7, numDescriptors=2)), " \
"CBV(b12, visibility = SHADER_VISIBILITY_ALL)," \
RS_STATIC_SAMPLER



cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBAmbient : register(b10)
{
	AmbientData g_ambient;
}

cbuffer CBLightingOption : register(b11)
{
	LightingOptionData g_lightingOp;
}

cbuffer CBLightCount : register(b12)
{
	LightCountData g_lightCount;
}

// ライト配列 : LightManager が毎フレーム詰め直したもの
StructuredBuffer<PointLight> g_pointLights : register(t7);
StructuredBuffer<DirectionalLight> g_directionalLights : register(t8);

// ディファードレンダリングでは共通
Texture2D g_albedoTex : register(t0);
Texture2D g_normalTex : register(t1);
Texture2D g_materialTex : register(t2);
Texture2D g_emiTex : register(t3);
Texture2D g_depthTex : register(t4);
Texture2D g_shadowMask : register(t5);
Texture2D g_rayGI : register(t6);

// 出力
RWTexture2D<float4> g_output : register(u0); // 結果書き込み用

// サンプラー
SamplerState g_samp : register(s0);

// ヘルパー関数 : 上で宣言した g_camera / g_ambient を使うので、必ずこの位置で読むこと
#include "../../../Common/Math/Transform.hlsli"
#include "../../../Common/Math/Normal.hlsli"
#include "../../../Common/Lighting/Fog.hlsli"

float3 ReconstructViewPos(float2 uv, float depth)
{
	float4 clip = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth, 1.0f);
	float4 view = mul(clip, g_camera.invProj);
	return view.xyz / view.w;
}

[RootSignature(DEFERRED_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画像の解像度を取得
	uint _width, _height;
	g_output.GetDimensions(_width, _height);
	
	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height)
		return;

	// 座標を計算
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height); // UV
	int2 _centerCoord = int2(DTid.xy); // センター座標
	
	// GBufferから情報を取得
	float3 _albedo = g_albedoTex.Load(int3(_centerCoord, 0)).rgb; // アルベド
	float _arpha = g_albedoTex.Load(int3(_centerCoord, 0)).a; // アルファ
	float2 _enc = g_normalTex.Load(int3(_centerCoord, 0)).rg; // 法線
	float3 _normal = DecsodeNormal(_enc); // 法線を復元
	float _depth = g_depthTex.Load(int3(_centerCoord, 0)).r; // 深度
	float _metallic = g_materialTex.Load(int3(_centerCoord, 0)).b; // 金属度
	float _roughness = g_materialTex.Load(int3(_centerCoord, 0)).g; // 粗さ

	float3 _emissive = g_emiTex.Load(int3(_centerCoord, 0)).rgb; // エミッシブ(自己発光)

	float _shadow = g_shadowMask.Load(int3(_centerCoord, 0)).r; // 影
	float3 _rayGI = g_rayGI.Load(int3(_centerCoord, 0)).rgb; // GI

	// 3D空間での位置を復元
	float3 _viewPos = ReconstructViewPos(_uv, _depth);
	float4 _worldPos4 = mul(float4(_viewPos, 1), g_camera.invView);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;

	//float3 _specular = _albedo; // スペキュラはアルベドと同じにしておく（今回はスペキュラを考慮しないため）
	// 非金属の基本反射率(F0)はオプションから調整可能にする
	float3 _F0 = lerp(
		g_lightingOp.dielectricF0.xxx,
		_albedo,
		_metallic
	);
	float _smoothness = 1.0f - _roughness; // 滑らかさ
	
	float3 _V = normalize(g_camera.cameraPos.xyz - _worldPos); // カメラ位置からワールド位置へのベクトル

	// 出力色
	float3 _outColor = float3(0, 0, 0);
	
	//------------------------------------------------------------------
	// 平行光
	//
	// 影を受けるのは先頭の1つだけ。
	// 影マスク(g_shadowMask)は1チャンネルしかなく、RaytracingShadowPass も
	// 主光源へレイを1本飛ばしているだけなので、2つ目以降へ同じマスクを掛けると
	// 別の光源が落とした影がそのまま乗ってしまう
	//------------------------------------------------------------------
	for (uint _dlIdx = 0; _dlIdx < g_lightCount.directionalNum; ++_dlIdx)
	{
		DirectionalLight _dl = g_directionalLights[_dlIdx];

		float3 _L = normalize(-_dl.dir);			// 光源に向かうベクトル
		float _NdotL = saturate(dot(_normal, _L));

		float3 _radiance = _dl.color.rgb * _dl.brightness;

		// 主光源だけがレイトレの影を受ける
		float _dlShadow = (_dlIdx == 0) ? _shadow : 1.0f;

		// シンプルなディズニーベースの拡散反射を実装する
		// フレネル反射を考慮した拡散反射を計算
		float _diffuseFromFresnel = CalcDiffuseFromFresnel(
			_normal,
			_L,
			_V,
			_roughness
		);

		// 正規化Lambert拡散反射を求める
		float3 _lambertDiffuse = _radiance * _NdotL / PI * _dlShadow;

		// 最終的な拡散反射光を計算
		float3 _diffuse = _albedo * _diffuseFromFresnel * _lambertDiffuse;

		// Cook-Torranceモデルの鏡面反射BRDF( D*F*G / (4*NdotL*NdotV) )を計算
		float _specTerm = CookTorranceSpecular(
			_L,
			_V,
			_normal,
			_metallic,
			_roughness
		);

		// レンダリング方程式の cosθ(=NdotL) を掛ける。
		// これが抜けていると BRDF の分母に残る 1/NdotL が NdotL→0 の明暗境界(ターミネータ)で
		// 発散し、球の側面に明るいリング状の模様が出たり、ハイライトが過剰に大きく/明るくなる。
		// (拡散反射側は _lambertDiffuse に NdotL が入っているが、鏡面側には掛かっていなかった)
		// あわせて float3 で受け、色付きライト/F0の色が正しく反映されるようにする。
		float3 _spec = _specTerm * _NdotL;
		_spec *= _radiance;
		_spec *= _dlShadow;

		// 金属度が高ければ、鏡面反射はF0(スペキュラカラー)、低ければ白
		_spec *= lerp(float3(1.0f, 1.0f, 1.0f), _F0, _metallic);

		// 直接光(拡散+鏡面)の強さをオプションから調整可能にする
		_outColor += (_diffuse + _spec) * g_lightingOp.directionalIntensity;
	}

	//------------------------------------------------------------------
	// 点光源
	//
	// 平行光と違って影を持たないので _shadow は掛けない。
	// directionalIntensity も太陽側のつまみなので掛けない
	//------------------------------------------------------------------
	for (uint _i = 0; _i < g_lightCount.pointNum; ++_i)
	{
		PointLight _pl = g_pointLights[_i];

		float3 _toLight = _pl.pos - _worldPos;
		float _distSq = dot(_toLight, _toLight);

		// 届く範囲の外と、面の裏側は計算ごと飛ばす
		if (_distSq >= _pl.range * _pl.range) continue;

		float _dist = sqrt(_distSq);
		float3 _plL = _toLight / max(_dist, 1e-4f);	// 光源に向かうベクトル
		float _plNdotL = saturate(dot(_normal, _plL));
		if (_plNdotL <= 0.0f) continue;

		// 距離減衰 : 逆二乗に「range で 0 になる窓」を掛ける。
		// 逆二乗だけだと range の境目で光が途切れて輪郭が出る。
		// 分母の +1 は光源に近づいたときに発散させないため
		float _window = saturate(1.0f - pow(_dist / _pl.range, 4.0f));
		float _atten = (_window * _window) / (_distSq + 1.0f);

		float3 _plRadiance = _pl.color.rgb * _pl.brightness * _atten;

		// 拡散反射 : 平行光と同じ式
		float _plDiffuseFromFresnel = CalcDiffuseFromFresnel(_normal, _plL, _V, _roughness);
		float3 _plDiffuse = _albedo * _plDiffuseFromFresnel * (_plRadiance * _plNdotL / PI);

		// 鏡面反射 : 平行光と同じくレンダリング方程式の NdotL を掛ける
		// (掛けないと BRDF の分母に残る 1/NdotL が明暗境界で発散する)
		float _plSpecTerm = CookTorranceSpecular(_plL, _V, _normal, _metallic, _roughness);
		float3 _plSpec = _plSpecTerm * _plNdotL * _plRadiance;

		// 金属度が高ければ鏡面反射はF0(スペキュラカラー)、低ければ白
		_plSpec *= lerp(float3(1.0f, 1.0f, 1.0f), _F0, _metallic);

		_outColor += _plDiffuse + _plSpec;
	}

	// アンビエント(GI/間接光) : 強さをオプションから調整可能にする
	_outColor += _rayGI * _albedo * g_lightingOp.giIntensity;

	// エミッシブ(自己発光)
	// 面が自分で出している光なので、影やライトの向きの影響を受けずそのまま足す。
	// GBufferEmissiv は R11G11B10_FLOAT なので 1.0 を超える値もそのまま乗る。
	_outColor += _emissive;

	// フォグ
	// ライティングが終わった色に対して、カメラからの深度(ビュー空間Z)と
	// ワールドYを見て掛ける。無効なら AmbientData の enable で丸ごとスキップされる。
	// 何も描かれていない画素は深度が最遠なので、そのままフォグ色で埋まる(地平線のかすみ)。
	_outColor = ApplyFog(_outColor, _viewPos.z, _worldPos.y);

	g_output[_centerCoord] = float4(_outColor, 1);
}
