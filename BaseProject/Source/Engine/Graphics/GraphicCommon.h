#pragma once

enum class RenderQueueType
{
	Shadow,
	Simple,
	Opaque,
	AnimationOpaque,
	Transparent,
	AnimationTransparent,
	Bloom,
	Lighting,
	Debug
};

enum class RenderQueueType2D
{
	ScreenUI,
	WorldUI
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
	None,
	SRV,
	RTV,
	Depth_Read,
	Depth_Write,
	UAV,
	CopySrc,
	CopyDst
};

struct AccessResource
{
	Engine::Resource::ID id = Engine::Resource::Limits::INVALID_ID;

	AccessType type = AccessType::None;
	LoadOp load = LoadOp::Clear;
	StoreOp store = StoreOp::Store;
};

struct ResourceHandle
{
	Engine::Resource::ID id;
	uint32_t version;
};

struct PassDesc
{
	// パスの識別名
	std::string name = "none";

	UINT rootSigID = UINT_MAX;		// パスが使用するルートシグネチャID
	Engine::Handle<Engine::D3D12::PipelineState> psoID;			// パスが使用するパイプラインステートID

	// 依存関係・トポロジカルソート用
	std::vector<Engine::Resource::ID> readResource = {};		// 入力(SRV)
	std::vector<Engine::Resource::ID> writeResource = {};	// 出力(RTV,UAV)

	// レンダーパス開始・終了時のAPI設定
	std::vector<AccessResource> resourceAccessVec = {};
};
