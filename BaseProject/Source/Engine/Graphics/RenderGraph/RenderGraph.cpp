#include "RenderGraph.h"
#include "RGVarsionManager/RGResourceManager.h"

#include "../RenderPass/DrawPass/ForwardLightingPass/ForwardLightingPass.h"
#include "../RenderPass/OffScreenPass/FullScreenPass/FullScreenPass.h"
#include "../RenderPass/DrawPass/ZPrePass/ZPrePass.h"
#include "../RenderPass/DrawPass/GBufferPass/GBufferPass.h"
#include "../RenderPass/DrawPass/AnimationGBufferPass/AnimationGBufferPass.h"
#include "../RenderPass/DrawPass/ScreenUIPass/ScreenUIPass.h"
#include "../RenderPass/OffScreenPass/DeferredLightingPass/DeferredLightingPass.h"
#include "../RenderPass/DrawPass/DebugLinePass/DebugLinePass.h"

#include "../RenderContext/RenderContext.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Resource/Manager/TextureManager/TextureManager.h"

namespace Engine::Graphics
{
	RenderGraph::RenderGraph()
	{}
	RenderGraph::~RenderGraph()
	{}

	void RenderGraph::Init(
		RenderContext* a_pCtx,
		Resource::ShaderManager* a_pShaderMana,
		RootSignatureManager* a_pRootSigMana,
		Engine::D3D12::GraphicsPSOManager* a_pPSOMana
	)
	{
		// リソースマネージャー作成
		m_upRGResourceManager = std::make_unique<RGResourceManager>();
		m_upRGResourceManager->Register(
			"MainColor",
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		);
		m_upRGResourceManager->Register(
			"QuadTexture",
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		);
		m_upRGResourceManager->Register(
			"GBufferAlbedo",
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		);
		m_upRGResourceManager->Register(
			"GBufferNormal",
			DXGI_FORMAT_R16G16_FLOAT,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		);
		m_upRGResourceManager->Register(
			"GBufferMaterial",
			DXGI_FORMAT_R8G8B8A8_UNORM,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		);
		m_upRGResourceManager->Register(
			"GBufferEmissiv",
			DXGI_FORMAT_R8G8B8A8_UNORM,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		);
		m_upRGResourceManager->Register(
			"Depth",
			DXGI_FORMAT_R32_TYPELESS,
			1280,
			720,
			Resource::TextureUsage::DSV | Resource::TextureUsage::SRV
		);

		//m_resourceStorage.Init(20);
		// リソース作成
		//CreateResource({
		//	.name = "MainColor",
		//	.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//	});
		//CreateResource({
		//	.name = "QuadTexture",
		//	.format = DXGI_FORMAT_R16G16B16A16_FLOAT,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//	});
		//CreateResource({
		//	.name = "GBufferAlbedo",
		//	.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//	});
		//CreateResource({
		//	.name = "GBufferNormal",
		//	.format = DXGI_FORMAT_R16G16_FLOAT,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//	});
		//CreateResource({
		//	.name = "GBufferMaterial",
		//	.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//	});
		//CreateResource({
		//	.name = "GBufferEmissiv",
		//	.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//	});
		//CreateResource({
		//	.name = "Depth",
		//	.format = DXGI_FORMAT_R32_TYPELESS,
		//	.widht = 1280,
		//	.height = 720,
		//	.usage = Resource::TextureUsage::DSV | Resource::TextureUsage::SRV
		//	});

		// パス登録
		RegisterPass<ZPrePass>();
		RegisterPass<GBufferPass>();
		RegisterPass<AnimationGBufferPass>();
		RegisterPass<DeferredLightingPass>();
		RegisterPass<ForwardLightingPass>();
		RegisterPass<FullScreenPass>();
		//RegisterPass<DebugLinePass>();
		//RegisterPass<ScreenUIPass>();

		// パスの初期化
		for (auto& _sp : m_spPassVec)
		{
			_sp->Init(this, a_pShaderMana, a_pRootSigMana, a_pPSOMana);
		}

		m_pCtx = a_pCtx;

		// コンパイル
		Compile();
	}

	void RenderGraph::Release()
	{
		m_compiledPasses.clear();
		//m_resourceStorage.Release();
		//m_rgResourceMap.clear();
		m_spPassVec.clear();
	}

	void RenderGraph::Compile()
	{
		m_compiledPasses.clear();

		// ソート配列の作成
		Algorithm::Graph::TopologicalSort(
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
		//for (auto& _pass : m_sortedPassed)
		//{

		//	for (const AccessResource& _access : _pass->GetDesc().resourceAccessVec)
		//	{
		//		if (_access.type == AccessType::SRV)
		//		{
		//			m_rgResourceMap[_access.id].desc.usage |= Resource::TextureUsage::SRV;
		//		}
		//		if (_access.type == AccessType::RTV)
		//		{
		//			m_rgResourceMap[_access.id].desc.usage |= Resource::TextureUsage::RTV;
		//		}
		//		if (_access.type == AccessType::UAV)
		//		{
		//			m_rgResourceMap[_access.id].desc.usage |= Resource::TextureUsage::UAV;
		//		}
		//		if (_access.type == AccessType::Depth_Read)
		//		{
		//			m_rgResourceMap[_access.id].desc.usage |= Resource::TextureUsage::DSV;
		//		}
		//		if (_access.type == AccessType::Depth_Write)
		//		{
		//			m_rgResourceMap[_access.id].desc.usage |= Resource::TextureUsage::DSV;
		//		}
		//	}
		//}
		//

		//// RGResourceの作成
		//for (auto& [_id, _res] : m_rgResourceMap)
		//{
		//	_res.texHandle = CreateTexture(
		//		_res.desc.name,
		//		_res.desc.format,
		//		_res.desc.widht,
		//		_res.desc.height,
		//		_res.desc.usage
		//	);
		//	_res.currentState = D3D12_RESOURCE_STATE_COMMON;
		//}

		// テクスチャの作成
		m_upRGResourceManager->CreateAllTexture();

		// リソース実態の作成
		for(auto* _pass : m_sortedPassed)
		{
			CompiledPass _cp;
			_cp.pPass = _pass;

			// リソース遷移作成
			for (auto _access : _pass->GetDesc().resourceAccessVec)
			{
				// クリア作成
				//auto& _res = m_rgResourceMap[_access.id];
				

				if (_access.type == AccessType::RTV)
				{
					//auto _rtv = GetRTVHandle(_res.texHandle);
					auto _rtv = m_upRGResourceManager->GetRTVHandle(_access.id);
					_cp.rtvHadles.push_back(_rtv);
					if (_access.load == LoadOp::Clear)
					{
						//_cp.clearRTVs.push_back(_res.texHandle);
						_cp.clearRTVs.push_back(m_upRGResourceManager->GetTexHandle(_access.id));
					}
				}
				else if (_access.type == AccessType::Depth_Write || 
						_access.type == AccessType::Depth_Read
					)
				{
					//_cp.dsvHandle = GetDSVHandle(_res.texHandle);
					_cp.dsvHandle = m_upRGResourceManager->GetDSVHandle(_access.id);
					if (_access.load == LoadOp::Clear)
					{
						_cp.isDepthClear = true;
					}
				}

				// バリア作成
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

				auto& _currentState = m_upRGResourceManager->RefCurrentState(_access.id);
				//if (_res.currentState != _next)
				if (_currentState != _next)
				{
					_cp.barrierVec.push_back(
						{
							m_upRGResourceManager->GetTexHandle(_access.id),
							_currentState,
							_next,
							_access.id
						}
					);
					//_cp.barrierVec.push_back(
					//	{
					//		_res.texHandle,
					//		_res.currentState,
					//		_next,
					//		_res.id
					//	}
					//);
					//_res.currentState = _next;
					_currentState = _next;
				}
			}

			// 実行データに入れていく
			m_compiledPasses.push_back(_cp);
			ImGuiContex::Instance().AddLog("PassName : %s\n", _cp.pPass->GetDesc().name.c_str());
		}
		
	}

	void RenderGraph::Excute(RenderContext* a_pCtx)
	{
		ImGuiContex::Instance().StartWatch("RenderGraphStart");

		// コンパイル済みパスを順次実行していく
		for (auto& _cp : m_compiledPasses)
		{
			// リソースバリア
			AutoBarrier(_cp);

			// リソースクリア
			// レンダーターゲット変更
			m_pCtx->ChangeRenderTarget(_cp.rtvHadles, _cp.dsvHandle);

			// クリア処理
			for (auto& _tex : _cp.clearRTVs)
			{
				m_pCtx->ClearRenderTarget(_tex);
			}
			if (_cp.isDepthClear)
			{
				m_pCtx->ClearDSV(_cp.dsvHandle);
			}

			// パスの実行
			_cp.pPass->Excute(a_pCtx);
		}

		ImGuiContex::Instance().EndWatch("RenderGraphStart");
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderGraph::GetGPUHandle(const std::string& a_name)
	{
		
		auto _tex = Resource::TextureManager::Instance().GetTexture(a_name);
		return DescriptorHeapManager::Instance().GetSRVGPUHandle(_tex.GetSRV());

	}

	//Engine::Resource::ID RenderGraph::CreateResource(const ResourceDesc& a_desc)
	//{
	//	// 持っているものを返す
	//	if (m_resourceStorage.Has(a_desc.name))
	//	{
	//		return m_resourceStorage.GetID(a_desc.name);
	//	}

	//	// 登録してかえす
	//	Engine::Resource::ID _id =
	//		m_resourceStorage.Add(a_desc.name, std::make_shared<ResourceDesc>(a_desc));

	//	RGResource _rgRes{};
	//	_rgRes.id = _id;
	//	_rgRes.desc = a_desc;
	//	_rgRes.currentState = D3D12_RESOURCE_STATE_COMMON;

	//	m_rgResourceMap[_id] = _rgRes;
	//	return _id;
	//}

	Engine::Resource::Handle<Engine::Resource::Texture> RenderGraph::CreateTexture(
		const std::string& a_name,
		const DXGI_FORMAT& a_format,
		const UINT64& a_widht,
		const UINT& a_height,
		const Resource::TextureUsage& a_texUsage
	)
	{
		Resource::CreateTextureDesc _desc = {
			.name = a_name,
			.width = a_widht,
			.height = a_height,
			.format = a_format,
			.usage = a_texUsage
		};
		return Resource::TextureManager::Instance().CreateTexture(_desc);
	}

	Resource::ID RenderGraph::Read(const std::string& a_resourceName, const AccessType& a_type)
	{
		switch (a_type)
		{
		case AccessType::SRV:
			return m_upRGResourceManager->Read(a_resourceName,Resource::TextureUsage::SRV);
		case AccessType::RTV:
			return m_upRGResourceManager->Read(a_resourceName, Resource::TextureUsage::RTV);
		case AccessType::UAV:
			return m_upRGResourceManager->Read(a_resourceName, Resource::TextureUsage::UAV);
		case AccessType::Depth_Read:
			return m_upRGResourceManager->Read(a_resourceName, Resource::TextureUsage::DSV);
		case AccessType::Depth_Write:
			return m_upRGResourceManager->Read(a_resourceName, Resource::TextureUsage::DSV);
		default:
			break;
		}

		return Resource::ID();
	}

	Resource::ID RenderGraph::Write(const std::string& a_resourceName, const AccessType& a_type)
	{
		switch (a_type)
		{
		case AccessType::SRV:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::SRV);
		case AccessType::RTV:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::RTV);
		case AccessType::UAV:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::UAV);
		case AccessType::Depth_Read:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::DSV);
		case AccessType::Depth_Write:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::DSV);
		default:
			break;
		}
		return Resource::ID();
	}

	//Engine::Resource::ID RenderGraph::GetID(const std::string& a_key)
	//{
	//	if (m_resourceStorage.Has(a_key))
	//	{
	//		return m_resourceStorage.GetID(a_key);
	//	}
	//	assert(0 && "登録されていないリソースです");
	//	return Engine::Resource::Limits::INVALID_ID;
	//}

	std::vector<std::string> RenderGraph::GetRGResourceList()
	{
		std::vector<std::string> _nameVec = {};

		//for (auto& [_id, _spRgTex] : m_rgResourceMap)
		//{
		//	_nameVec.push_back(_spRgTex.desc.name);
		//}

		return _nameVec;
	}

	//RGResource& RenderGraph::GetRGresource(const Engine::Resource::ID& a_id)
	//{
	//	auto _it = m_rgResourceMap.find(a_id);
	//	if (_it != m_rgResourceMap.end())
	//	{
	//		return _it->second;
	//	}

	//	assert(0 && "RGリソースが見つかりません %d", a_id);
	//}

	DXGI_FORMAT RenderGraph::GetDXGIFormat(Resource::ID a_id)
	{
		return m_upRGResourceManager->GetDXGIFormat(a_id);
	}

	void RenderGraph::AutoBarrier(CompiledPass& a_pass)
	{
		// 使用するリソースバリア
		for (auto& _barrier : a_pass.barrierVec)
		{
			// 現在のステートと変更予定ステートが違うのならば変更する
			//if (m_rgResourceMap[_barrier.resID].currentState != _barrier.after)
			if (m_upRGResourceManager->RefCurrentState(_barrier.resID) != _barrier.after)
			{
				// ステート変更
				m_pCtx->Transition(
					Resource::TextureManager::Instance().RefTexture(_barrier.texHandle).GetResource(),
				//	m_rgResourceMap[_barrier.resID].currentState,
					m_upRGResourceManager->RefCurrentState(_barrier.resID),
					_barrier.after
				);
				// 現在のステートを更新
				//m_rgResourceMap[_barrier.resID].currentState = _barrier.after;
				m_upRGResourceManager->RefCurrentState(_barrier.resID) = _barrier.after;
			}
		}
	}

	Resource::Handle<RTV> RenderGraph::GetRTVHandle(Resource::Handle<Resource::Texture> a_handle)
	{
		return Resource::TextureManager::Instance().GetTexture(a_handle).GetRTV();
	}
	Resource::Handle<DSV> RenderGraph::GetDSVHandle(Resource::Handle<Resource::Texture> a_handle)
	{
		return Resource::TextureManager::Instance().GetTexture(a_handle).GetDSV();
	}
	Resource::Handle<SRV> RenderGraph::GetSRVHandle(Resource::Handle<Resource::Texture> a_handle)
	{
		return Resource::TextureManager::Instance().GetTexture(a_handle).GetSRV();
	}
	Resource::Handle<SRV> RenderGraph::GetImGuiSRVHandle(Resource::Handle<Resource::Texture> a_handle)
	{
		return Resource::TextureManager::Instance().GetTexture(a_handle).GetImGuiSRV();
	}
}