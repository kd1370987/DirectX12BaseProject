#include "DebugLineRoot.hlsli"
// ルートシグネチャ定義
[RootSignature(DEBUGLINE_ROOT_SIG)]

VSOutput vs(VSInput a_input)
{
	int _index = g_bufferIndex.instanceDataIndex;
	
	VSOutput _outPut = (VSOutput)0;

	// 投影変換
	_outPut.svPos = Transform_LocalToProj(a_input.pos, g_instanceData[_index].worldMat);
	_outPut.color = a_input.color;
	
	return _outPut;
}
