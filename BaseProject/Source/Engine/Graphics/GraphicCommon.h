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
	Load,			// 前のパスで書かれていた
	Clear,			// 初回使用
	DontCare		// これ以上後ろで使われることがない
};

enum class StoreOp
{
	Store,
	DontCare		// これ以上後ろで使われることがない
};

enum class AccessType
{
	SRV,
	RTV,
	Depth_Read,
	Depth_Write,
	UAV
};

struct AccessResource
{
	Resource::ID id;
	AccessType type;

	LoadOp load;
	StoreOp store;
};

struct PassDesc
{
	// パスの識別名
	std::string name;

	UINT rootSigID;		// パスが使用するルートシグネチャID
	UINT psoID;			// パスが使用するパイプラインステートID

	// 依存関係・トポロジカルソート用
	std::vector<Resource::ID> readResource;		// 入力(SRV)
	std::vector<Resource::ID> writeResource;	// 出力(RTV,UAV)

	RenderQueueType queueType;		// 描画アイテムタイプ

	bool isCulled = false;		// 依存関係的に不要ならスキップ
	bool isAsync = false;		// 将来用

	Viewport viewport;
	ScissorRectangle scissor;

	// レンダーパス開始・終了時のAPI設定
	std::vector<AccessResource> resourceAccessVec;
};

#include "RootSigLayout/RootSigLayout.h"