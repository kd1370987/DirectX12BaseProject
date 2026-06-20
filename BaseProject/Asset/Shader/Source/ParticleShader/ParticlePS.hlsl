#include "ParticleShader.hlsli"

float4 PSMain(VSOutput a_input) : SV_TARGET
{
	float4 _outColor = float4(0,0,0,1);

	float4 _texColor = g_mainTex.Sample(g_samp, a_input.uv);
	
	_outColor = _texColor * a_input.color;

	// アルファ値がほぼ0ならピクセルを破棄して描画負荷を下げる
	if (_outColor.a < 0.001f)
	{
		discard;
	}

	// 定数バッファで発光強度を持たせておく
	//float intensity = 5.0f;
	//_outColor.rgb *= intensity;
	
	return _outColor;
}
