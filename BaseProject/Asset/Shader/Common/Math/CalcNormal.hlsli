
// 折り畳み・展開用補助関数
float2 SignNotZero(float2 v)
{
	return float2(
        v.x >= 0 ? 1.0 : -1.0,
        v.y >= 0 ? 1.0 : -1.0
    );
};

// 復元
// 下の処理で圧縮された２次元ノーマルを３次元に戻す
float3 DecsodeNormal(float2 a_f)
{
	// 必要に応じてＸＹを反転させる
	float3 _n = float3(a_f.xy, 1 - abs(a_f.x) - abs(a_f.y));
	if (_n.z < 0.0f)
	{
		_n.xy = (1 - abs(_n.yx)) * SignNotZero(_n.xy);
	}
	// ノーマライズして返す
	return normalize(_n);
};

// 圧縮
// ノーマルを書き込む際に３次元ノーマルから２次元ノーマルに変換する
float2 EncodeNormalOct(float3 a_n)
{
	// 絶対値の和が１になるように正規化
	a_n /= (abs(a_n.x) + abs(a_n.y) + abs(a_n.z));
	float2 _enc = a_n.xy;

	// Zがマイナス方向の場合XYを対角線上に反転させる
	if (a_n.z < 0)
	{
		_enc = (1 - abs(_enc.yx)) * SignNotZero(_enc);
	}
	
	return _enc;
};
