#include "Screen2DShader.hlsli"


// ルートシグネチャ定義
[RootSignature(SCREEN2D_ROOT_SIG)]

float4 PSMain(VSOutput a_input) : SV_Target
{
	float4 _outColor = float4(0, 0, 0, 1); // 出力色の初期化
	
	float4 _texColor = g_mainTex.Sample(g_smp, a_input.uv); // テクスチャの色をサンプリング

	_outColor = _texColor * color; // テクスチャの色を出力色に設定
	
	return _outColor;
}
