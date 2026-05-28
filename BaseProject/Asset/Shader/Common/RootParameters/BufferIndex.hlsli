// インクルードガード
#ifndef CB_BUFFERINDEX_HLSLI
#define CB_BUFFERINDEX_HLSLI

struct BufferIndex
{
	int instanceDataIndex;
	int subsetDataIndex;

	float pad1;
	float pad2;
};

// カメラの定数バッファ
cbuffer CBBufferIndex : register(b1)
{
	BufferIndex g_bufferIndex;
}

#endif

// 共通定数バッファ
#define RS_BUFFERINDEX_CB "CBV(b1,visibility = SHADER_VISIBILITY_ALL)"
