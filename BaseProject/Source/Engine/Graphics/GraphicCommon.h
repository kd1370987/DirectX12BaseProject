#pragma once

enum class RenderQueueType
{
	Shadow,
	Simple,
	Opaque,
	Transparent,
	GBuffer,
	Bloom,
	Lighting,
	Depth
};

enum class LoadOp
{
	Load,
	Clear,
	DontCare
};

enum class StoreOp
{
	Store,
	DontCare
};

// レンダーターゲットを使うときの処理
struct AttachementDesc
{
	RTVHandle _rtvHandle;
	LoadOp load;
	StoreOp store;
};