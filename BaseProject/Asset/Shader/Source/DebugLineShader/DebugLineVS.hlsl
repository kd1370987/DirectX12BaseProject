#include "../../RootSignatureIncl/DebugLineRoot.hlsli"

VSOutput vs(VSInput a_input)
{
	VSOutput _outPut = (VSOutput)0;

	// 投影変換
	float4 _localPos = float4(a_input.pos,1.0f);	// ローカル座標
	float4 _worldPos = mul(mat,_localPos);			// ワールド座標
	float4 _viewPos = mul(viewMat,_worldPos);		// ビュー座標
	float4 _projPos = mul(projMat,_viewPos);		// プロジェクション座標

	_outPut.svPos = _projPos;
	
	return _outPut;
}
