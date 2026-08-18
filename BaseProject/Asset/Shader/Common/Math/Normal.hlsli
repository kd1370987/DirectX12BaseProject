#ifndef NORMAL_HLSLI
#define NORMAL_HLSLI

//==========================================================================================
// 法線用の行列(逆転置)を作る
//------------------------------------------------------------------------------------------
// 座標は M をそのまま掛ければよいが、法線は (M^-1)^T を掛けないと面に垂直でなくなる。
// 等方スケールなら M を掛けて正規化しても向きは変わらないので今まで問題にならなかったが、
// 非等方スケール(例: glTFのノードに 50,1,50 が入っている地形)では
// 引き伸ばされた軸の成分がその倍率だけ強調され、法線が寝てしまう。
// (50,1,50 の場合、真上を向いていた法線がほぼ水平になり、陰影が完全に破綻する)
//
// 逆行列は要らない。余因子行列 C = det(M) * (M^-1)^T は、行ベクトル規約なら
// 「各行 = 他の2行の外積」で求まる。結果は det(M) 倍されるだけなので、
// 使う側で正規化すれば向きは (M^-1)^T と一致する。除算も分岐も無い。
//
// ※ 鏡映(det < 0)が入った行列では符号が反転して法線が裏返る。
//   このエンジンはインポート時に頂点と行列の両方をZミラーしており、
//   行列側は S*A*S の相似変換で行列式の符号が変わらないため、ここでは考慮しない。
//==========================================================================================
float3x3 GetNormalMatrix(float4x4 a_world)
{
	float3x3 _m = (float3x3) a_world;

	return float3x3(
		cross(_m[1], _m[2]),
		cross(_m[2], _m[0]),
		cross(_m[0], _m[1]));
}

// 法線をローカルからワールドに変換
float3 Normal_LocalToWorld(float3 a_normal, float4x4 a_worldMat)
{
	return normalize(mul(a_normal, GetNormalMatrix(a_worldMat)));
}

// 接線をローカルからワールドに変換
//
// 接線は面に「沿う」ベクトルなので、法線とは違いワールド行列をそのまま掛ける。
// ここを逆転置にすると、非等方スケールのモデルで接空間が法線とねじれてしまう。
float3 Tangent_LocalToWorld(float3 a_tangent, float4x4 a_worldMat)
{
	return normalize(mul(a_tangent, (float3x3) a_worldMat));
}

// ワールド行列のスケールが等方か
//
// 法線コーンのように「向きだけでなく開き角も意味を持つ」データは、
// 非等方スケールが掛かると正しく移せない。使う側で判定を諦めるために使う。
bool IsUniformScale(float4x4 a_worldMat)
{
	float _sx = length(a_worldMat[0].xyz);
	float _sy = length(a_worldMat[1].xyz);
	float _sz = length(a_worldMat[2].xyz);

	float _minScale = min(_sx, min(_sy, _sz));
	float _maxScale = max(_sx, max(_sy, _sz));

	// 誤差の範囲は許す(1%)
	return _maxScale <= _minScale * 1.01f;
}

#endif
