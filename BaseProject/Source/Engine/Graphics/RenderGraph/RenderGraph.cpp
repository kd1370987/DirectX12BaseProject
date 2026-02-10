#include "RenderGraph.h"

#include "../RenderPass/DrawPass/ForwardLightingPass/ForwardLightingPass.h"
#include "../RenderPass/OffScreenPass/FullScreenPass/FullScreenPass.h"
#include "../RenderPass/DrawPass/ZPrePass/ZPrePass.h"
#include "../RenderPass/DrawPass/GBufferPass/GBufferPass.h"
#include "../RenderPass/DrawPass/AnimationGBufferPass/AnimationGBufferPass.h"
#include "../RenderPass/OffScreenPass/DeferredLightingPass/DeferredLightingPass.h"

#include "../RenderContext/RenderContext.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void RenderGraph::Init(ShaderManager* a_pShaderMana, RootSignatureManager* a_pRootSigMana, GraphicsPSOManager* a_pPSOMana)
{
	m_resourceStorage.Init(20);
	// リソース作成


	CreateResource({
		.name = "MainColor",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.widht = 1280,
		.height = 720,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
	});
	CreateResource({
		.name = "QuadTexture",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.widht = 1280,
		.height = 720,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
	});

	CreateResource({
		.name = "GBufferAlbedo",
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.widht = 1280,
		.height = 720,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
		});
	CreateResource({
		.name = "GBufferNormal",
		.format = DXGI_FORMAT_R16G16_FLOAT,
		.widht = 1280,
		.height = 720,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
		});
	CreateResource({
		.name = "GBufferMaterial",
		// R = Roughness, G = Matallic, B = Specular or AO , A = EmissiveMask
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.widht = 1280,
		.height = 720,
		.usage = ResourceUsage::ShaderRead | ResourceUsage::RenderTarget
	});
	CreateResource({
		.name = "Depth",
		.format = DXGI_FORMAT_R32_TYPELESS,
		.widht = 1280,
		.height = 720,
		.usage = ResourceUsage::DepthStencil | ResourceUsage::ShaderRead
	});

	// パス登録
	RegisterPass<ForwardLightingPass>();
	RegisterPass<ZPrePass>();
	RegisterPass<FullScreenPass>();
	RegisterPass<GBufferPass>();
	RegisterPass<AnimationGBufferPass>();
	RegisterPass<DeferredLightingPass>();

	// パスの初期化
	for (auto& _sp : m_spPassVec)
	{
		_sp->Init(this,a_pShaderMana,a_pRootSigMana,a_pPSOMana);
	}

	// コンパイル
	Compile();
}

void RenderGraph::Release()
{
	m_compiledPasses.clear();
	m_resourceStorage.Release();
	m_rgResourceMap.clear();
	m_sortedPassed.clear();
	m_spPassVec.clear();
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

	// リソース作成用にリソースの使い方を収集
	for (auto* _pass : m_sortedPassed)
	{
		for (const AccessResource& _access : _pass->GetDesc().resourceAccessVec)
		{
			if (_access.type == AccessType::SRV)
			{
				m_rgResourceMap[_access.id].desc.usage |= ResourceUsage::ShaderRead;
			}
			if (_access.type == AccessType::RTV)
			{
				m_rgResourceMap[_access.id].desc.usage |= ResourceUsage::RenderTarget;
			}
			if (_access.type == AccessType::UAV)
			{
				m_rgResourceMap[_access.id].desc.usage |= ResourceUsage::ShaderWrite;
			}
			if (_access.type == AccessType::Depth_Read)
			{
				m_rgResourceMap[_access.id].desc.usage |= ResourceUsage::DepthStencil;
			}
			if (_access.type == AccessType::Depth_Write)
			{
				m_rgResourceMap[_access.id].desc.usage |= ResourceUsage::DepthStencil;
			}
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
		_desc.allowDSV = HasFlag(_res.desc.usage, ResourceUsage::DepthStencil);

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

		// リソース遷移作成
		for (auto _access : _pass->GetDesc().resourceAccessVec)
		{
			D3D12_RESOURCE_STATES _next = D3D12_RESOURCE_STATE_COMMON;
			if (_access.type == AccessType::SRV)
			{
				_next = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			if (_access.type == AccessType::RTV)
			{
				_next = D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
			if (_access.type == AccessType::UAV)
			{
				_next = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
			if (_access.type == AccessType::Depth_Read)
			{
				_next = D3D12_RESOURCE_STATE_DEPTH_READ;
			}
			if (_access.type == AccessType::Depth_Write)
			{
				_next = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			}

			auto& _res = m_rgResourceMap[_access.id];
			if (_res.currentState != _next)
			{
				_cp.barrierVec.push_back(
					{
						_res.spRGTexture.get(),
						_res.currentState,
						_next,
						_res.id
					}
				);
				_res.currentState = _next;
			}
		}

		// 実行データに入れていく
		m_compiledPasses.push_back(_cp);
	}
}

void RenderGraph::Excute(RenderContext* a_pCtx)
{
	/*ImGuiContex::Instance().AddLog("===============================================================================\n");
	ImGuiContex::Instance().AddLog("Frame開始\n");
	ImGuiContex::Instance().AddLog("===============================================================================\n");*/

	ImGuiContex::Instance().StartWatch("RenderGraphStart");

	int m_barrierSize = 0;

	// コンパイル済みパスを順次実行していく
	for (auto& _cp : m_compiledPasses)
	{
		// 使用するリソースバリア
		for (auto& _barrier : _cp.barrierVec)
		{
			if(m_rgResourceMap[_barrier.resID].currentState != _barrier.after)
			{
				a_pCtx->Transition(
					_barrier.texture->GetResource(),
					m_rgResourceMap[_barrier.resID].currentState,
					_barrier.after
				);
			/*	ImGuiContex::Instance().AddLog(
					"Resource : %s\n",
					m_rgResourceMap[_barrier.resID].desc.name.c_str()
				);
				ImGuiContex::Instance().AddLog(
					"beffor = % s, affter = % s\n",
					magic_enum::enum_name(m_rgResourceMap[_barrier.resID].currentState).data(),
					magic_enum::enum_name(_barrier.after).data()
				);*/
				m_rgResourceMap[_barrier.resID].currentState = _barrier.after;

				m_barrierSize++;
			}
		}

		// RTV
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _rtvs = {};
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _clearRTVs = {};

		// DSV
		D3D12_CPU_DESCRIPTOR_HANDLE _dsv = {};
		D3D12_CPU_DESCRIPTOR_HANDLE* _pDsv = nullptr;
		bool _isClear = false;
		
		for (auto& _resAcc : _cp.pPass->GetDesc().resourceAccessVec)
		{
			// レンダーターゲット
			if (_resAcc.type == AccessType::RTV)
			{
				auto& _tex = m_rgResourceMap[_resAcc.id].spRGTexture;
				_rtvs.push_back(_tex->GetRTVHandle());
				// クリアを入れるかどうか
				if (_resAcc.load == LoadOp::Clear)
				{
					_clearRTVs.push_back(_tex->GetRTVHandle());
					//ImGuiContex::Instance().AddLog("ClearRTV:%s\n", m_rgResourceMap[_resAcc.id].desc.name.c_str());
				}
				else
				{
					//ImGuiContex::Instance().AddLog("rtv:%s\n", m_rgResourceMap[_resAcc.id].desc.name.c_str());
				}
				
			}

			// 深度値
			if (_resAcc.type == AccessType::Depth_Write || 
				_resAcc.type == AccessType::Depth_Read)
			{
				auto& _depth = m_rgResourceMap[_resAcc.id].spRGTexture;
				_dsv = _depth->GetDSVHandle();
				_pDsv = &_dsv;
				// クリアを入れるかどうか
				if (_resAcc.load == LoadOp::Clear)
				{
					_isClear = true;
					//ImGuiContex::Instance().AddLog("ClearDSV:%s\n", m_rgResourceMap[_resAcc.id].desc.name.c_str());
				}
				else
				{
					///ImGuiContex::Instance().AddLog("DSV:%s\n", m_rgResourceMap[_resAcc.id].desc.name.c_str());
				}
			}
		}
	
		// レンダーターゲットを変更
		a_pCtx->ChangeRenderTarget(_rtvs,_pDsv);

		// クリア
		for (auto& _handle : _clearRTVs)
		{
			a_pCtx->ClearRenderTarget(_handle);
		}
		if (_isClear)
		{
			a_pCtx->ClearDepth(_dsv);
		}

		// パスの実行
		_cp.pPass->Excute(a_pCtx);

		//ImGuiContex::Instance().AddLog("===============================================================================\n");
		
	}
	ImGuiContex::Instance().AddLog("%d\n",m_barrierSize);
	/*ImGuiContex::Instance().AddLog("===============================================================================\n");
	ImGuiContex::Instance().AddLog("Frame終了\n");
	ImGuiContex::Instance().AddLog("===============================================================================\n");*/
	ImGuiContex::Instance().EndWatch("RenderGraphStart");
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderGraph::GetGPUHandle(const std::string& a_name)
{
	auto _id = m_resourceStorage.GetID(a_name);
	return m_rgResourceMap[_id].spRGTexture->GPUSRVHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::RTVCPU(const std::string& a_name)
{
	auto _id = m_resourceStorage.GetID(a_name);
	return m_rgResourceMap[_id].spRGTexture->GetRTVHandle();
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
	return Resource::Limits::INVALID_ID;
}
