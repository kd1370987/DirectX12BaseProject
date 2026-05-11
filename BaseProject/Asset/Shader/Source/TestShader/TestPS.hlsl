#include "Test.hlsli"

// ルートシグネチャ定義
[RootSignature(TEST_ROOT_SIG)]

#include "../CalcNormal.hlsli"

float4 ps(VSOutput a_input) : SV_Target
{
	float2 _uv = a_input.uv;
	
	float4 _out; 

	// --------------------------------------------------
	// 1. アルベド
	// --------------------------------------------------
	// ヒープからインデックスを使ってTexture2Dを取り出し、サンプリングする
	Texture2D albedoMap = ResourceDescriptorHeap[texIndex.x];
	float4 _baseTex = albedoMap.Sample(smp, _uv);
	_out = _baseTex * baseColor;

	
	return float4(0, 1, 0, 1);
	return _out;
}
