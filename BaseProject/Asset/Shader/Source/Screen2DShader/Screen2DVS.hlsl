#include "Screen2DShader.hlsli"

// ルートシグネチャ定義
[RootSignature(SCREEN2D_ROOT_SIG)]

VSOutput VSMain(VSInput a_input )
{
	VSOutput _output = (VSOutput) 0; // アウトプット構造体を定義
	
	_output.svPos = a_input.pos;	// 頂点座標
	_output.svPos = mul(mat, _output.svPos);	// 変換
	_output.uv = a_input.uv; // uv座標をそのままピクセルシェーダーに渡す
	
	return _output;
}
