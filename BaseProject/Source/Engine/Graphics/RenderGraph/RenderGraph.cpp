#include "RenderGraph.h"

void RenderGraph::Init()
{

}

void RenderGraph::Compile()
{
}

void RenderGraph::Excute(RenderContext* a_pCtx)
{
	for (auto* _excute : m_sortedPassed)
	{
		_excute->Excute(a_pCtx);
	}
}

Resource::ID RenderGraph::CreateResource(const ResourceDesc& a_desc)
{
	// 持っているものを返す
	if (m_resourceStorage.Has(a_desc.name))
	{
		return m_resourceStorage.GetID(a_desc.name);
	}

	// 登録してかえす
	return m_resourceStorage.Add(a_desc.name,std::make_shared<ResourceDesc>(a_desc));
}

Resource::ID RenderGraph::GetID(const std::string& a_key)
{
	if (m_resourceStorage.Has(a_key))
	{
		return m_resourceStorage.GetID(a_key);
	}
	assert(0 && "登録されていないリソースです");
}
