#pragma once

#include "../RenderPass/RenderPass.h"

enum class ResourceUsage : uint32_t
{
	None = 0,
	RenderTarget = 1 << 0,
	DepthStencil = 1 << 1,
	ShaderRead = 1 << 2,
	ShaderWrite = 1 << 3,   // UAV 用
};

struct ResourceDesc
{
	std::string name;
	DXGI_FORMAT format;
	ResourceUsage usage;
};

class RenderGraph
{
public:

	
	void Init();							// 初回

	void Compile();							// Pass追加後
	void Excute(RenderContext* a_pCtx);		// パスを順次実行

	Resource::ID CreateResource(const ResourceDesc& a_desc);
	Resource::ID GetID(const std::string& a_key);


private:
	std::vector<RenderPass*> m_sortedPassed;			// ソート後のパス
	std::unordered_map<Resource::ID, RenderPass*> m_resourceProducer;		// このリソースを最後に書いたパスは誰
	std::unordered_map<RenderPass*, std::vector<RenderPass*>> m_edges;		// パスA→パスBへの依存関係

	// 実態は持たないが、どのリソース、いつ作られた、いつ破棄予定を計算するためのもの
	std::unordered_map<Resource::ID, ResourceDesc> m_resource;

	SlotStorage<ResourceDesc> m_resourceStorage;
};