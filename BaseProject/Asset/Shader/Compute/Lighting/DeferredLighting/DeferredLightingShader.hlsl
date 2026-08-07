#include "../../../Source/CalcNormal.hlsli"
#include "../../../Source/CalcLighting.hlsli"
#include "../../../Source/RootSignatureLayout.hlsli"

#include "../../../Common/Math/Transform.hlsli"
#include "../../../Common/Math/Normal.hlsli"

// ルートパラメターズ
#include "../../../Common/CB/CBCamera.hlsli"
#include "../../../Common/RootParameters/AmbientData.hlsli"
#include "../../../Common/CB/CBLightingOption.hlsli"

// ルートシグネチャデータ
// ※末尾に追加することで、既存のルートパラメータ番号(SRVテーブル=2, UAVテーブル=3)を
//   ずらさずに、ライティング調整用CBをルートパラメータ番号4として足している。
#define DEFERRED_ROOT_SIG \
"RootFlags(0)," \
RS_CAMERA_CB "," \
RS_AMBIENT_CB "," \
"DescriptorTable(SRV(t0, numDescriptors=7)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_LIGHTING_OPTION_CB "," \
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
	_spec *= g_ambient.DL_Color;
	_spec *= _shadow;

	// 金属度が高ければ、鏡面反射はF0(スペキュラカラー)、低ければ白
	_spec *= lerp(float3(1.0f, 1.0f, 1.0f), _F0, _metallic);

	// 直接光(拡散+鏡面)の強さをオプションから調整可能にする
	_outColor += (_diffuse + _spec) * g_lightingOp.directionalIntensity;

	// アンビエント(GI/間接光) : 強さをオプションから調整可能にする
	_outColor += _rayGI * _albedo * g_lightingOp.giIntensity;

	// エミッシブ(自己発光)
	// 面が自分で出している光なので、影やライトの向きの影響を受けずそのまま足す。
	// GBufferEmissiv は R11G11B10_FLOAT なので 1.0 を超える値もそのまま乗る。
	_outColor += _emissive;

	g_output[_centerCoord] = float4(_outColor, 1);
}
