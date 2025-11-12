cbuffer Transform : register(b0)
{
    float4x4 World;     // ワールド行列
    float4x4 View;      // ビュー行列
    float4x4 Proj;      // 投影行列
}

struct VSInput
{
    float3 pos : POSITION;     // 頂点座標
    float3 normal : NORMAL;     // 法線
    float2 uv : TEXCOORD;       // uv座標
    float3 tangent : TANGENT;   // 接空間
    float4 color : COLOR;       // 頂点色
};

struct VSOutput
{
    float4 svpos	: SV_Position;		// 変換された座標
    float4 color	: COLOR;			// 変換された色
	float2 uv		: TEXCOORD;			// uv座標
};

VSOutput vert(VSInput a_input)
{
    VSOutput _output = (VSOutput) 0;                    // アウトプット構造体を定義
    
    float4 _localPos = float4(a_input.pos, 1.0f);       // 頂点座標
    float4 _worldPos = mul(World, _localPos);           // ワールド座標に変換
    float4 _viewPos = mul(View, _worldPos);             // ビュー座標に変換
    float4 _projPos = mul(Proj, _viewPos);              // 投影変換
    
    _output.svpos = _projPos;           // 投影変換された座標をピクセルシェーダーに渡す
    _output.color = a_input.color;      // 頂点色をそのままピクセルシェーダーに渡す
	_output.uv = a_input.uv;	// uv座標をそのままピクセルシェーダーに渡す
    
    return _output;
}
