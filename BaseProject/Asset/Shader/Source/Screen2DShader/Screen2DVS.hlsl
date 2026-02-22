#include "Screen2DShader.hlsli"

VSOutput vs(VSInput a_input )
{
	VSOutput _output = (VSOutput) 0; // アウトプット構造体を定義

	_output.svPos = float4(a_input.pos, 1.0f);	// 頂点座標
	_output.svPos = mul(mat, _output.svPos);	// 変換
	_output.uv = a_input.uv; // uv座標をそのままピクセルシェーダーに渡す
	
	return _output;
}
