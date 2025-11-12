struct VSOutput
{
    float4 svpos	: SV_Position;		// 頂点シェーダーからきた座標
    float4 color	: COLOR;			// 頂点シェーダーから来た色
	float2 uv		: TEXCOORD;			// 頂点シェーダーから来たuv座標
};

SamplerState smp : register(s0);		// サンプラー
Texture2D _MainTex : register(t0);			// テクスチャ

float4 pixel(VSOutput a_input) : SV_Target
{
	return _MainTex.Sample(smp, a_input.uv);
}
