#include "DeferredLightingShader.hlsli"

// ルートシグネチャ定義
[RootSignature(DEFERRED_ROOT_SIG)]

VSOutput VSMain(uint a_id : SV_VertexID)
{
	VSOutput _out;

	float2 _uv = float2(
        (a_id << 1) & 2,
        a_id & 2
    );

	_out.uv = _uv;
	_out.pos = float4(_uv * float2(2, -2) + float2(-1, 1), 0, 1);

	return _out;
}
