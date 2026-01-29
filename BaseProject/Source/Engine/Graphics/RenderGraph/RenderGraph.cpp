#include "RenderGraph.h"

#include "../RenderPass/DrawPass/ForwardLightingPass/ForwardLightingPass.h"

void RenderGraph::Init(ShaderManager* a_pShaderMana, RootSignatureManager* a_pRootSigMana, GraphicsPSOManager* a_pPSOMana)
{
	m_resourceStorage.Init(20);

	// リソース作成
	CreateResource({
		.name = "BuckBuffer",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.usage = ResourceUsage::ShaderRead
	});
	CreateResource({
		.name = "Depth",
		.format = DXGI_FORMAT_R32_FLOAT,
		.usage = ResourceUsage::ShaderRead
	});
	CreateResource({
		.name = "MainColor",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.usage = ResourceUsage::ShaderRead
	});
	CreateResource({
		.name = "QuadTexture",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.usage = ResourceUsage::ShaderRead
	});

	RegisterPass<ForwardLightingPass>();

	for (auto _sp : m_spPassVec)
	{
		_sp->Init(this,a_pShaderMana,a_pRootSigMana,a_pPSOMana);
	}
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
