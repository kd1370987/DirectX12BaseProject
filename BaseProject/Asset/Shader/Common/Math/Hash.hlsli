#ifndef HASH_HLSLI
#define HASH_HLSLI

static const float PI = 3.14159265f;

// 完全な乱数ではなくパターンが見える場合がある
// メモリ不要で非常に高速
float Hash(uint a_seed)
{
	a_seed = (a_seed << 13u) ^ a_seed;
	return frac(
        (a_seed * (a_seed * a_seed * 15731u + 789221u) + 1376312589u)
        / 65536.0f);
}

// 分布が非常に良くパターンが見えにくい
uint PCGHash(uint a_seed)
{
	uint _state = a_seed * 747796405u + 2891336453u;
	uint _word =
        ((_state >> ((_state >> 28u) + 4u)) ^ _state)
        * 277803737u;
	return (_word >> 22u) ^ _word;
}

// ハッシュ化されたランダムな値を取得する
float Random(uint a_seed)
{
	return PCGHash(a_seed) / 4294967295.0;
}

// ランダムな方向を取得する
float3 RandomDirection(uint a_seed)
{
	float _z = Random(a_seed++) * 2.0f - 1.0f;
	float _a = Random(a_seed++) * 6.2831853f;

	float _r = sqrt(1.0f - _z * _z);

	return float3(
        _r * cos(_a),
        _r * sin(_a),
        _z);
}

// 軸となるベクトルからコーン上に角度を指定してランダムなベクトルを取得する
float3 RandomConeDirection(
	float3 a_forward,
	float a_angleRad,
	uint a_seed
)
{
	float _cosTheta = lerp(
		cos(a_angleRad),
		1.0f,
		Random(a_seed++)
	);

	float _sinTheta = sqrt(1.0f - _cosTheta * _cosTheta);

	float _phi = Random(a_seed++) * 2.0f * PI;

 // ローカル空間
	float3 _localDir =
	{
		cos(_phi) * _sinTheta,
        sin(_phi) * _sinTheta,
        _cosTheta
	};

    // forward に直交するベクトルを作る
	float3 _up =
        abs(a_forward.y) < 0.999
        ? float3(0, 1, 0)
        : float3(1, 0, 0);

	float3 _right = normalize(cross(_up, a_forward));
	_up = cross(a_forward, _right);

    // ローカル→ワールド変換
	return normalize(
        _right * _localDir.x +
        _up * _localDir.y +
		a_forward * _localDir.z
	);

}

// ２点間のランダムな値を取得
// シードは uint で受け取ること。float で受けると 2^24 を超えた値が
// 丸められて別々のシードが同じ値に潰れる。
float ValueFloat(float a_min, float a_max, uint a_seed)
{
	return lerp(a_min, a_max, Random(a_seed));
}

#endif
