Texture2D<float4> g_tex : register(t0);
Texture2D<float4> g_ui : register(t1);

SamplerState g_smp : register(s0);

struct Output
{
	float4 svPos : SV_POSITION;
	float2 uv : TEXCOORD;
};
