#include "RenderGraph.h"

#include "../RenderPass/DrawPass/ForwardLightingPass/ForwardLightingPass.h"
#include "../RenderPass/OffScreenPass/FullScreenPass/FullScreenPass.h"

#include "../RenderContext/RenderContext.h"

#include "../../D3D12/D3D12Wrapper/RenderingEngine.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void RenderGraph::Init(ShaderManager* a_pShaderMana, RootSignatureManager* a_pRootSigMana, GraphicsPSOManager* a_pPSOMana)
{
	m_resourceStorage.Init(20);

	// リソース作成
	CreateResource({
		.name = "BackBuffer",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
	});
	CreateResource({
		.name = "Depth",
		.format = DXGI_FORMAT_R32_FLOAT,
		.usage = ResourceUsage::ShaderRead
	});
	CreateResource({
		.name = "MainColor",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
	});
	CreateResource({
		.name = "QuadTexture",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.usage = ResourceUsage::ShaderRead
	});

	RegisterPass<FullScreenPass>();
	RegisterPass<ForwardLightingPass>();

	for (auto _sp : m_spPassVec)
	{
		_sp->Init(this,a_pShaderMana,a_pRootSigMana,a_pPSOMana);
	}

	Compile();
}

void RenderGraph::Compile()
{
	m_compiledPasses.clear();

	// ソート配列の作成
	Graph::TopologicalSort(
		m_spPassVec,
		m_sortedPassed,
		[&](auto& a, auto& b)
		{
			// 依存があるかどうか
			for (auto& _write : b.GetDesc().writeResource)
			{
				for (auto& _read : a.GetDesc().readResource)
				{
					if (_write == _read)
					{
						return true;
					}
				}
			}
			return false;
		}
	);

	// リソース用作ようにリソースの使い方を収集
	for (auto* _pass : m_sortedPassed)
	{
		for (auto _id : _pass->GetDesc().readResource)
		{
			m_rgResourceMap[_id].desc.usage |= ResourceUsage::ShaderRead;
		}
		for (auto _id : _pass->GetDesc().writeResource)
		{
			m_rgResourceMap[_id].desc.usage |= ResourceUsage::ShaderWrite;
		}
	}

	// RGResourceの作成
	for (auto& [_id, _res] : m_rgResourceMap)
	{
		RGTextureDesc _desc = {};
		_desc.width = _res.desc.widht;
		_desc.height = _res.desc.height;
		_desc.format = _res.desc.format;
		
		_desc.allowSRV = HasFlag(_res.desc.usage,ResourceUsage::ShaderRead);
		_desc.allowRTV = HasFlag(_res.desc.usage,ResourceUsage::RenderTarget);
		_desc.allowUAV = HasFlag(_res.desc.usage,ResourceUsage::ShaderWrite);
		_desc.allowDSV = HasFlag(_res.desc.usage,ResourceUsage::DepthStencil);

		_res.spRGTexture = std::make_shared<RGTexture>();
		_res.spRGTexture->Create(_desc);

		_res.currentState = D3D12_RESOURCE_STATE_COMMON;
	}

	// リソース実態の作成
	for (uint32_t _passIdx = 0; _passIdx < m_sortedPassed.size(); ++_passIdx)
	{
		auto* _pass = m_sortedPassed[_passIdx];
		CompiledPass _cp;
		_cp.pPass = _pass;

		auto _ProcesssRes = [&](Resource::ID a_id, bool a_isWrite)
			{
				auto& _res = m_rgResourceMap[a_id];
				//auto _next = ToD3DState(_res.desc.usage, a_isWrite);
				D3D12_RESOURCE_STATES _next;
				if (!a_isWrite)
				{
					//_next = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
					_next = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				}
				else
				{
					_next = D3D12_RESOURCE_STATE_RENDER_TARGET;
				}

				if (_res.currentState != _next)
				{
					_cp.barrierVec.push_back(
						{
							_res.spRGTexture.get(),
							_res.currentState,
							_next
						}
					);
					_res.currentState = _next;
				}

				if (a_isWrite)
				{
					_res.lastWritePass = _passIdx;
				}
			};

		for (auto _id : _pass->GetDesc().readResource)
		{
			_ProcesssRes(_id,false);
		}
		for (auto _id : _pass->GetDesc().writeResource)
		{
			_ProcesssRes(_id, true);
		}

		// 実行データに入れていく
		m_compiledPasses.push_back(_cp);
	}
}

void RenderGraph::Excute(RenderContext* a_pCtx)
{
	for (auto& _cp : m_compiledPasses)
	{
		for (auto& _barrier : _cp.barrierVec)
		{

			assert(_barrier.texture->GetResource() != nullptr);

			a_pCtx->Transition(
				_barrier.texture->GetResource(),
				_barrier.before,
				_barrier.after
			);
		}

		// RTV
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _rtvs;
		for (auto& _att : _cp.pPass->GetDesc().colorAttachements)
		{
			auto& _tex = m_rgResourceMap[_att.id].spRGTexture;
			assert(_tex->GetRTVHandle().ptr != 0);
			_rtvs.push_back(_tex->GetRTVHandle());
		}

		// DSV
		D3D12_CPU_DESCRIPTOR_HANDLE* _dsv = nullptr;
		if (_cp.pPass->GetDesc().depthAttachement.has_value())
		{
			auto& _tex = m_rgResourceMap[_cp.pPass->GetDesc().depthAttachement->id].spRGTexture;
			_dsv = new D3D12_CPU_DESCRIPTOR_HANDLE;
			*_dsv = DescriptorHeapManager::Instance().GetCPUDSV();
		}
		

		// レンダーターゲットを変更
		a_pCtx->ChangeRenderTarget(_rtvs,_dsv);

		// クリア
		auto _size = _cp.pPass->GetDesc().colorAttachements.size();
		for (size_t _i = 0; _i < _size; ++_i)
		{
			if (_cp.pPass->GetDesc().colorAttachements[_i].load == LoadOp::Clear)
			{
				a_pCtx->ClearRenderTarget(_rtvs[_i]);
			}
		}
		if (_cp.pPass->GetDesc().depthAttachement && _cp.pPass->GetDesc().depthAttachement->load == LoadOp::Clear)
		{
			if(_dsv)
			{
				a_pCtx->ClearDepth(*_dsv);
			}
		}

		// パスの実行
		_cp.pPass->Excute(a_pCtx);

		if (_dsv) delete _dsv;
	}
	auto _id = m_resourceStorage.GetID("BackBuffer");
	D3D12_GPU_DESCRIPTOR_HANDLE _final = m_rgResourceMap[_id].spRGTexture->GPUSRVHandle();
	a_pCtx->DrawQuad(_final);
}

Resource::ID RenderGraph::CreateResource(const ResourceDesc& a_desc)
{
	// 持っているものを返す
	if (m_resourceStorage.Has(a_desc.name))
	{
		return m_resourceStorage.GetID(a_desc.name);
	}

	// 登録してかえす
	Resource::ID _id = 
		m_resourceStorage.Add(a_desc.name, std::make_shared<ResourceDesc>(a_desc));

	RGResource _rgRes{};
	_rgRes.id = _id;
	_rgRes.desc = a_desc;
	_rgRes.currentState = D3D12_RESOURCE_STATE_COMMON;

	m_rgResourceMap[_id] = _rgRes;

	return _id;
}

Resource::ID RenderGraph::GetID(const std::string& a_key)
{
	if (m_resourceStorage.Has(a_key))
	{
		return m_resourceStorage.GetID(a_key);
	}
	assert(0 && "登録されていないリソースです");
}

D3D12_RESOURCE_STATES RenderGraph::ToD3DState(ResourceUsage a_usage, bool a_isWrite)
{
	if (a_isWrite)
	{
		if (HasFlag(a_usage ,ResourceUsage::RenderTarget))
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (HasFlag(a_usage, ResourceUsage::DepthStencil))
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (HasFlag(a_usage, ResourceUsage::ShaderWrite))
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else
	{
		if (HasFlag(a_usage, ResourceUsage::ShaderRead))
			return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		if (HasFlag(a_usage, ResourceUsage::DepthStencil))
			return D3D12_RESOURCE_STATE_DEPTH_READ;
	}

	return D3D12_RESOURCE_STATE_COMMON;
}

void RenderGraph::ProcessRes(Resource::ID a_id, bool a_isWrite)
{

}
