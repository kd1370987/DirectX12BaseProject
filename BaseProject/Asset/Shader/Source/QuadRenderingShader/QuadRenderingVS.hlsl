#include "QuadRenderingShader.hlsli"

Output vs( float4 a_pos : POSITION ,float2 a_uv : TEXCOORD) 
{
	Output _output;
	_output.svPos = a_pos;
	_output.uv = a_uv;
	return _output;
}
