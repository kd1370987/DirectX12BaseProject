#include "../RootSignatureLayout.hlsli"

#define QUADRENDERING_ROOT_SIG \
RS_FLAGS","\
"DescriptorTable(SRV(t0, numDescriptors=1),visibility = SHADER_VISIBILITY_PIXEL), " \
"DescriptorTable(SRV(t1, numDescriptors=1),visibility = SHADER_VISIBILITY_PIXEL), " \
RS_STATIC_SAMPLER


Texture2D<float4> g_tex : register(t0);
Texture2D<float4> g_ui : register(t1);

SamplerState g_smp : register(s0);

struct Output
{
	float4 svPos : SV_POSITION;
	float2 uv : TEXCOORD;
};
