// デバッグ形状1つぶん。頂点は持たず、shapeType から VS が線を組み立てる。
// ※ CPU 側 Engine::Graphics::DebugLineData と並びを合わせること
#ifndef ROOTPARAM_DEBUG_LINE_DATA_HLSLI
#define ROOTPARAM_DEBUG_LINE_DATA_HLSLI

struct DebugLineData
{
	float4 color;
	float4x4 worldMat;
	uint shapeType;
};

#endif
