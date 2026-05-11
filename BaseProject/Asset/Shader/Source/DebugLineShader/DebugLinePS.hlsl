#include "DebugLineRoot.hlsli"
// ルートシグネチャ定義
[RootSignature(DEBUGLINE_ROOT_SIG)]

float4 ps(VSOutput a_input) : SV_TARGET
{
	float4 _outColor = { 1.0f, 0.0f, 1.0f, 1.0f };
	
	return _outColor;
}
