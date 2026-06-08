#include "../../../Source/CalcNormal.hlsli"
#include "../../../Source/CalcLighting.hlsli"
#include "../../../Source/RootSignatureLayout.hlsli"

#include "../../../Common/Math/Transform.hlsli"
#include "../../../Common/Math/Normal.hlsli"

// ルートパラメターズ
#include "../../../Common/CB/CBCamera.hlsli"
#include "../../../Common/RootParameters/AmbientData.hlsli"

// ルートシグネチャデータ
#define DEFERRED_ROOT_SIG \
"RootFlags(0)," \
RS_CAMERA_CB "," \
RS_AMBIENT_CB "," \
"DescriptorTable(SRV(t0, numDescriptors=7)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER



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

	float _shadow = g_shadowMask.Load(int3(_centerCoord, 0)).r; // 影
	float3 _rayGI = g_rayGI.Load(int3(_centerCoord, 0)).rgb; // GI

	// 3D空間での位置を復元
	float3 _viewPos = ReconstructViewPos(_uv, _depth);
	float4 _worldPos4 = mul(float4(_viewPos, 1), g_camera.invView);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;

	//float3 _specular = _albedo; // スペキュラはアルベドと同じにしておく（今回はスペキュラを考慮しないため）
	float3 _F0 = lerp(
		float3(0.04, 0.04, 0.04),
		_albedo,
		_metallic
	);
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
	//_spec *= lerp(float3(1.0f, 1.0f, 1.0f), _specular, _metallic);
	_spec *= lerp(float3(1.0f, 1.0f, 1.0f), _F0, _metallic);

	// 滑らかさを考慮した拡散反射を計算
	_outColor += _diffuse * (1.0f) + _spec;

	// アンビエント
	_outColor += _rayGI * _albedo;
	
	g_output[_centerCoord] = float4(_outColor, 1);
}
