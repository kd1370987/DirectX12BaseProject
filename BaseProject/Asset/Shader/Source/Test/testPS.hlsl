
float4 PSMain(
float4 Pos : SV_Position,
float3 Color : COLOR
) : SV_Target
{
    // MSから送られてきた色をそのまま出力
	return float4(Color, 1.0);
}
