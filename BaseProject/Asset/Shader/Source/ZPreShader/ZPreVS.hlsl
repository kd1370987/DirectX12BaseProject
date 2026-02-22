#include "ZPreShader.hlsli"

VSOutput vs(VSInput a_input)
{
	// スキニング
	row_major float4x4 _mBones = 0;
	[unroll]
	for (int _i = 0; _i < 4; ++_i)
	{
		_mBones += g_mBones[a_input.skinIndex[_i]] * a_input.skinWeight[_i];
	}

	// 座標と法線に適用
	float4 skinnedPos = mul(float4(a_input.pos, 1), _mBones);
	a_input.pos = skinnedPos.xyz;
	
	VSOutput _out;
	float4 _wPos = mul(mat, float4(a_input.pos,1));
	_out.svpos = mul(cView,_wPos);
	_out.svpos = mul(cProj,_out.svpos);
	
	return _out;
}
