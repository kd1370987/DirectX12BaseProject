#include "GBufferShader.hlsli"

// 頂点シェーダー入出力構造体
struct VSInput
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // uv座標
	float3 tangent : TANGENT; // 接空間
	float4 color : COLOR; // 頂点色
};

// ルートシグネチャ定義
[RootSignature(GBUFFER_ROOT_SIG)]

VSOutput VSMain(VSInput a_input)
{
	int _index = g_bufferIndex.instanceDataIndex;
	float4x4 _worldMat = g_instanceData[_index].worldMat;
	float4x4 _prevWorldMat = g_instanceData[_index].prevWorldMat;
	
	// 出力用構造体
	VSOutput _output = (VSOutput) 0; // アウトプット構造体を定義
	// クリップ
	float4 _projPos = Transform_LocalToProj(a_input.pos, _worldMat);

	_output.svpos = _projPos;			// 投影変換された座標をピクセルシェーダーに渡す
	_output.color = a_input.color;		// 頂点色をそのままピクセルシェーダーに渡す
	_output.uv = a_input.uv;			// uv座標をそのままピクセルシェーダーに渡す
	_output.normal = a_input.normal;	// 法線をそのままピクセルシェーダーに渡す

	// ワールド法線、接線、副接線
	float3 _binormal = cross(a_input.normal, a_input.tangent.xyz);
	_output.wT = Normal_LocalToWorld(a_input.tangent.xyz, _worldMat);
	_output.wB = Normal_LocalToWorld(_binormal, _worldMat);
	_output.wN = Normal_LocalToWorld(a_input.normal, _worldMat);

	// モーションベクター用
	_output.curClipPos = Transform_NonJitteredLocalToProj(a_input.pos, _worldMat);
	_output.prevClipPos = Transform_PrevLocalToProj(a_input.pos, _prevWorldMat);
	//_output.prevClipPos = Transform_PrevLocalToProj(a_input.pos, _worldMat);
	
	return _output;
}
