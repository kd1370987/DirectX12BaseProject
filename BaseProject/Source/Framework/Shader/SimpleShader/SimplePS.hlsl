struct VSOutput
{
    float4 svpos : SV_Position;     // 頂点シェーダーからきた座標
    float4 color : COLOR;           // 頂点シェーダーから来た色
};

float4 pixel(VSOutput a_input) : SV_Target
{
    return a_input.color;       // 色をそのまま表示
}