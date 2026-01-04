#include "SimpleShader.hlsli"

VSOutput vert(VSInput a_input)
{
    VSOutput _output = (VSOutput) 0;                    // アウトプット構造体を定義
    
    float4 _localPos = float4(a_input.pos, 1.0f);       // 頂点座標
    float4 _worldPos = mul(mat, _localPos);           // ワールド座標に変換
    float4 _viewPos = mul(cView, _worldPos);             // ビュー座標に変換
    float4 _projPos = mul(cProj, _viewPos);              // 投影変換
    
    _output.svpos = _projPos;           // 投影変換された座標をピクセルシェーダーに渡す
    _output.color = a_input.color;      // 頂点色をそのままピクセルシェーダーに渡す
  //  _output.color = a_input.color + baseColor;      // 頂点色をそのままピクセルシェーダーに渡す
	_output.uv = a_input.uv;	// uv座標をそのままピクセルシェーダーに渡す
    
    return _output;
}
