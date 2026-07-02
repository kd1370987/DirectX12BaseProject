#include "QuadRenderingShader.hlsli"


// ルートシグネチャ定義
[RootSignature(QUADRENDERING_ROOT_SIG)]

Output VSMain(uint a_id : SV_VertexID)
{
	Output _output;
	float2 _uv = float2(
        (a_id << 1) & 2,
        a_id & 2
    );

	_output.uv = _uv;
	_output.svPos = float4(_uv * float2(2, -2) + float2(-1, 1), 0, 1);
	
	return _output;
}
