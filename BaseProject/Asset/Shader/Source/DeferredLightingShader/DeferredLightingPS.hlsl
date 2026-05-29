#include "DeferredLightingShader.hlsli"
#include "../CalcLighting.hlsli"
#include "../CalcNormal.hlsli"

// ルートシグネチャ定義
[RootSignature(DEFERRED_ROOT_SIG)]

float3 ReconstructViewPos(float2 uv, float depth)
{
	float4 clip = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth, 1.0f);
	float4 view = mul(clip, g_camera.invProj);
	return view.xyz / view.w;
}


float4 ps(VSOutput a_in) : SV_Target
{	
	// GBufferから情報を取得
	float3 _albedo = g_albedoTex.Sample(g_samp, a_in.uv).rgb;	// アルベド
	float _arpha = g_albedoTex.Sample(g_samp, a_in.uv).a;		// アルファ
	float2 _enc = g_normalTex.Sample(g_samp, a_in.uv).rg;		// 法線
	float3 _normal = DecsodeNormal(_enc);						// 法線を復元
	float _depth = g_depthTex.Sample(g_samp, a_in.uv).r;		// 深度
	float _metallic = g_materialTex.Sample(g_samp, a_in.uv).b;	// 金属度
	float _roughness = g_materialTex.Sample(g_samp, a_in.uv).g; // 粗さ

	float _shadow = g_shadowMask.Sample(g_samp,a_in.uv).r;	// 影
	float3 _rayGI = g_rayGI.Sample(g_samp,a_in.uv).rgb;		// GI
	
	// 3D空間での位置を復元
	float3 _viewPos = ReconstructViewPos(a_in.uv, _depth);
	float4 _worldPos4 = mul(float4(_viewPos, 1), g_camera.invView);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;

	float3 _specular = _albedo; // スペキュラはアルベドと同じにしておく（今回はスペキュラを考慮しないため）
	float _smoothness = 1.0f - _roughness; // 滑らかさ
	
	float3 _V = normalize(g_camera.cameraPos.xyz - _worldPos); // カメラ位置からワールド位置へのベクトル

	// 出力色
	float3 _outColor = float3(0, 0, 0);
	
    // 平行光
	float3 _L = normalize(-g_ambient.DL_Dir.xyz);
	float _NdotL = saturate(dot(_normal, _L));

	// シンプルなディズニーベースの拡散反射を実装する
	// フレネル販社を考慮した拡散反射を計算
	float _diffuseFromFresnel = CalcDiffuseFromFresnel(
		_normal,
		_L,
		_V,
		_roughness
	);

	// 正規化Lambert拡散反射を求める
	float3 _lambertDiffuse = g_ambient.DL_Color * _NdotL / PI * _shadow;

	// 最終的な拡散反射光を計算
	float3 _diffuse = _albedo * _diffuseFromFresnel * _lambertDiffuse;

	// Cook-Torranceモデルを利用した鏡面反射を計算
	float _spec = CookTorranceSpecular(
		_L,
		_V,
		_normal,
		_metallic,
		_roughness
	);
	_spec *= g_ambient.DL_Color;
	_spec *= _shadow;

	// 金属度が高ければ、鏡面反射はスペキュラカラー、低ければ白
	_spec *= lerp(float3(1.0f, 1.0f, 1.0f), _specular, _metallic);

	// 滑らかさを考慮した拡散反射を計算
	_outColor += _diffuse * (1.0f) + _spec;

	// アンビエント
	_outColor += _rayGI * _albedo;
	
	return float4(_outColor, _arpha);
}
