#include "ZPreShader.hlsli"

VSOutput vs(VSInput a_input)
{
	VSOutput _out;
	float4 _wPos = mul(mat, float4(a_input.pos,1));
	_out.svpos = mul(cView,_wPos);
	_out.svpos = mul(cProj,_out.svpos);
	
	return _out;
}
