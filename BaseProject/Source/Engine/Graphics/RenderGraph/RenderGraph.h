#pragma once

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

enum class RenderQueueType
{
	Opaque,			// 不透明
	Transparent,	// 透明
	Shadow,			// 影
	PostEffect		// ポストエフェクト
};

struct ResourceDesc
{
	std::string name;
	DXGI_FORMAT format;
	uint32_t width;
	uint32_t height;
};

// レンダーターゲットを使うときの処理
struct AttachementDesc
{
	Resource::ID resource;
	LoadOp load;
	StoreOp store;
};

struct PassDesc
{
	std::string name;

	UINT rootSigID;
	UINT psoID;

	std::vector<Resource::ID> readResource;		// 入力(SRV,Mesh,Vertex)
	std::vector<Resource::ID> writeResource;	// 出力(RTV,UAV)

	RenderQueueType queueType;

	bool isCulled = false;		// 依存関係的に不要ならスキップ
	bool isAsync = false;		// 将来用

	std::vector<AttachementDesc> colorAttachements;
	std::optional<AttachementDesc> depthAttachement;
};

class RenderPass
{
public:

	virtual ~RenderPass() = default;
	const PassDesc& GetDesc() const
	{
		return m_passDesc;
	}

	virtual void Excute(RenderContext* a_ctx) = 0;

protected:
	PassDesc m_passDesc;
};

class RenderGraph
{
public:

	void Compile();		// Pass追加後
	void Excute();		// パスを順次実行


private:
	std::vector<RenderPass*> m_sortedPassed;			// ソート後のパス
	std::unordered_map<Resource::ID, RenderPass*> m_resourceProducer;		// このリソースを最後に書いたパスは誰
	std::unordered_map<RenderPass*, std::vector<RenderPass*>> m_edges;		// パスA→パスBへの依存関係

	// 実態は持たないが、どのリソース、いつ作られた、いつ破棄予定を計算するためのもの
	std::unordered_map<Resource::ID, ResourceDesc> m_resource;
};