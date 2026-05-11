
float2 SignNotZero(float2 v)
{
	return float2(
        v.x >= 0 ? 1.0 : -1.0,
        v.y >= 0 ? 1.0 : -1.0
    );
};

float3 DecsodeNormal(float2 a_f)
{
	float3 _n = float3(a_f.xy, 1 - abs(a_f.x) - abs(a_f.y));
	if (_n.z < 0.0f)
	{
		_n.xy = (1 - abs(_n.yx)) * SignNotZero(_n.xy);
	}
	return normalize(_n);
};

float2 EncodeNormalOct(float3 a_n)
{
	a_n /= (abs(a_n.x) + abs(a_n.y) + abs(a_n.z));
	float2 _enc = a_n.xy;

	if (a_n.z < 0)
	{
		_enc = (1 - abs(_enc.yx)) * SignNotZero(_enc);
	}
	
	return _enc;
};
