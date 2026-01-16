#include "PBRShader.hlsli"

VSOutput vert(VSInput a_input)
{
	VSOutput _output = (VSOutput) 0; // アウトプット構造体を定義
    
	float4 _localPos = float4(a_input.pos, 1.0f);	// 頂点座標
	float4 _worldPos = mul(mat, _localPos);			// ワールド座標に
	_output.wPos = _worldPos.xyz;	// ワールド座標をピクセルシェーダーに渡す
	float4 _viewPos = mul(cView, _worldPos);		// ビュー座標に変換
	float4 _projPos = mul(cProj, _viewPos);			// 投影変換
	_output.svPos = _projPos; // 投影変換された座標をピクセルシェーダーに渡す

	// 頂点色
	_output.color = a_input.color;

	// ワールド法線、接線、副接線
	_output.wN = normalize(mul((float3x3) mat, a_input.normal));
	_output.wT = normalize(mul((float3x3) mat, a_input.tangent.xyz));

	float3 _binormal = cross(a_input.normal, a_input.tangent.xyz);
	_output.wB = normalize(mul((float3x3) mat, _binormal));

	// uv座標
	_output.uv = a_input.uv * uvTransform.zw + uvTransform.xy; 
    
	return _output;
}
