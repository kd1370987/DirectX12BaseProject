#include "DebugLineRoot.hlsli"
// ルートシグネチャ定義
[RootSignature(DEBUGLINE_ROOT_SIG)]

float4 ps(VSOutput a_input) : SV_TARGET
{
	return a_input.color;
}
