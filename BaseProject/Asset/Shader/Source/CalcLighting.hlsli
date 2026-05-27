
// 定数
static const float PI = 3.1415926f;


//===== フレネル反射を考慮した拡散反射を計算 =====
// この関数はフレネル販社を考慮した拡散反射率を計算します。
// フレネル反射がつよければ拡散反射は弱くなり、逆もあります。
// フレネル反射 = 光が物体の表面で反射する現象
// 拡散反射 = 光が物体の表面で散乱する現象
// @param a_N 法線ベクトル
// @param a_L 光源に向かうベクトル
// @param a_V 視点に向かうベクトル
float CalcDiffuseFromFresnel(float3 a_N, float3 a_L, float3 a_V, float a_roughness)
{
	// 光源に向かうベクトルと視線に向かうベクトルのハーフベクトルを求める
	float3 _H = normalize(a_L + a_V);

	// 粗さは0.5で固定しておく
	float _roughness = a_roughness;

	// エネルギーバイアス = 粗さが大きいほど、フレネル反射が強いほど、拡散反射が弱くなるようにするための値
	float _energyBias = lerp(0.0, 0.5, _roughness);
	// エネルギーファクター = 粗さが大きいほど、フレネル反射が強いほど、拡散反射が弱くなるようにするための値
	float _energyFactor = lerp(1.0, 1.0 / 1.51, _roughness);

	// 光源に向かうベクトルとハーフベクトルがどれだけ似ているかを内積を利用して求める
	float _dotLH = saturate(dot(a_L, _H));

	// 光源に向かうベクトルとハーフベクトル、光が平行に入社した時の拡散反射量を求める
	float _fd90 = _energyBias + 2.0 * _dotLH * _dotLH * _roughness;

	// 法線と光源に向かうベクトルWを利用して、拡散反射率を求める
	float _dotNL = saturate(dot(a_N, a_L));
	float _FL = (1 + (_fd90 - 1) * pow(1 - _dotNL, 5));

	// 法線と視線に向かうベクトルを利用して、拡散反射率を求める
	float _dotNV = saturate(dot(a_N, a_V));
	float _FV = (1 + (_fd90 - 1) * pow(1 - _dotNV, 5));

	// 法線と光源への方向に依存する拡散反射率と、法線と支店ベクトルに依存する拡散反射率を
	// 乗算して最終的な拡散反射率を求める
	return (_FL * _FV * _energyFactor);
};


//===== ベックマン分布を用いてD項を計算 =====
// @param a_m 粗さ
// @param a_NdotH 法線とハーフベクトルの内積
float Beckmann(float a_m, float a_t)
{
	float _t2 = a_t * a_t;
	float _t4 = a_t * a_t * a_t * a_t;
	float _m2 = a_m * a_m;
	float _D = 1.0f / (4.0f * _m2 * _t4);
	_D *= exp((-1.0f / _m2) * (1.0f - _t2) / _t2);
	return _D;
};


//===== Schlick近似を用いてフレネル反射を計算 =====
// @param a_f0 垂直入射の時のフレネル反射率
// @param a_u ライトに向かうベクトルとハーフベクトルの内積
float SpcFresnel(float a_f0, float a_u)
{
	return a_f0 + (1 - a_f0) * pow(1 - a_u, 5);
};


//===== Cook-Torranceモデルの鏡面反射を計算 =====
// @param a_L 光源に向かうベクトル
// @param a_V 視点に向かうベクトル
// @param a_N 法線ベクトル
// @param a_metallic 金属度
float CookTorranceSpecular(float3 a_L, float3 a_V, float3 a_N, float a_metallic, float a_roughness)
{
	// 金属度を垂直入射の時のフレネル反射率として扱う
	float _f0 = a_metallic;

	// ライトに向かうベクトルと視線に向かうベクトルのハーフベクトルを求める
	float3 _H = normalize(a_L + a_V);

	// 各種ベクトルがどれくらい似ているかを内積を利用して求める
	float _NdotH = saturate(dot(a_N, _H));
	float _VdotH = saturate(dot(a_V, _H));
	float _NdotL = saturate(dot(a_N, a_L));
	float _NdotV = saturate(dot(a_N, a_V));

	// D項をベックマン分布を用いて計算する
	float _D = Beckmann(max(a_roughness,0.05f), _NdotH);

	// F項をSchlick近似を用いて計算する
	_f0 = lerp(0.04f, 1.0f, a_metallic);
	float _F = SpcFresnel(_f0, _VdotH);

	// G項をSmithの方法を用いて計算する
	float _G = min(1.0f, min((2.0f * _NdotH * _NdotV) / max(_VdotH, 0.001f), (2.0f * _NdotH * _NdotL) / max(_VdotH, 0.001f)));

	// m項を計算する
	float _denom = 4.0f * _NdotL * _NdotV + 0.0001f; // ゼロ割り防止のための微小値
	
	// Cook-Torranceモデルの鏡面反射を計算する
	return max(_F * _D * _G / _denom, 0.0f);
};
