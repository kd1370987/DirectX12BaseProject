#include "RenderGraph.h"
#include "RGVarsionManager/RGResourceManager.h"

#include "../RenderPass/RasterizePass/ForwardLightingPass/ForwardLightingPass.h"
#include "../RenderPass/RasterizePass/FullScreenPass/FullScreenPass.h"
#include "../RenderPass/RasterizePass/ZPrePass/ZPrePass.h"
#include "../RenderPass/RasterizePass/GBufferPass/GBufferPass.h"
#include "../RenderPass/RasterizePass/ScreenUIPass/ScreenUIPass.h"
#include "../RenderPass/RasterizePass/DeferredLightingPass/DeferredLightingPass.h"
#include "../RenderPass/RasterizePass/DebugLinePass/DebugLinePass.h"
#include "../RenderPass/RasterizePass/TestPass/TestPass.h"

#include "../RenderPass/RaytracingPass/RaytracingShadowPass/RaytracingShadowPass.h"

#include "../RenderContext/RenderContext.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

//#include "../../Resource/Manager/TextureManager/TextureManager.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Resource/Loader/Texture/TextureLoader.h"

namespace Engine::Graphics
{
	RenderGraph::RenderGraph()
	{}
	RenderGraph::~RenderGraph()
	{}

	void RenderGraph::Init(
		D3D12::RootSignatureManager* a_pRootSigMana,
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
			"UITexture",
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			1280,
			720,
			Resource::TextureUsage::SRV | Resource::TextureUsage::RTV,
			{0,0,0,0}
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
		// レイの結果用
		m_upRGResourceManager->Register(
			"RayShadow",
			DXGI_FORMAT_R8G8B8A8_UNORM,
			1280,
			720,
			Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		);
		m_upRGResourceManager->Register(
			"Test",
			DXGI_FORMAT_R8G8B8A8_UNORM,
			1280,
			720,
			Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		);

		// パス登録
		RegisterPass<ZPrePass>();
		RegisterPass<GBufferPass>();
		RegisterPass<DeferredLightingPass>();
		//RegisterPass<ForwardLightingPass>();
		RegisterPass<FullScreenPass>();
		RegisterPass<DebugLinePass>();
		RegisterPass<ScreenUIPass>();

		RegisterPass<TestPass>();

		RegisterPass<RaytracingShadowPass>();

		// パスの初期化
		for (auto& _sp : m_spPassVec)
		{
			PassInitDesc _desc = {};
			_desc.pRG = this;
			_desc.pRootSigMana = a_pRootSigMana;
			_desc.pPSOMana = a_pPSOMana;
			_sp->Init(_desc);
		}

		// コンパイル
		Compile();
	}

	void RenderGraph::Release()
	{
		m_compiledPasses.clear();
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
				for (auto& _write : b.GetWrite())
				{
					for (auto& _read : a.GetRead())
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

		// テクスチャの作成
		m_upRGResourceManager->CreateAllTexture();

		// リソース実態の作成
		for(auto* _pass : m_sortedPassed)
		{
			CompiledPass _cp;
			_cp.pPass = _pass;

			// リソース遷移作成
			for (auto _access : _pass->GetResourceAccessVec())
			{
				// クリア作成
				if (_access.type == AccessType::RTV)
				{
					auto _rtv = m_upRGResourceManager->GetRTVHandle(_access.id);
					_cp.rtvHadles.push_back(_rtv);
					if (_access.load == LoadOp::Clear)
					{
						_cp.clearRTVs.push_back(m_upRGResourceManager->GetTexHandle(_access.id));
					}
				}
				else if (_access.type == AccessType::Depth_Write || 
						_access.type == AccessType::Depth_Read
					)
				{
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
					_currentState = _next;
				}
			}

			// 実行データに入れていく
			m_compiledPasses.push_back(_cp);
		}
		
		m_upRGResourceManager->StateReset();
	}

	void RenderGraph::Excute(RenderContext* a_pCtx)
	{
		// コンパイル済みパスを順次実行していく
		for (auto& _cp : m_compiledPasses)
		{
			// リソースバリア
			AutoBarrier(a_pCtx,_cp);

			// リソースクリア
			// レンダーターゲット変更
			a_pCtx->ChangeRenderTarget(_cp.rtvHadles, _cp.dsvHandle);

			// クリア処理
			for (auto& _tex : _cp.clearRTVs)
			{
				a_pCtx->ClearRenderTarget(_tex);
			}
			if (_cp.isDepthClear)
			{
				a_pCtx->ClearDSV(_cp.dsvHandle);
			}

			// パスの実行
			_cp.pPass->Excute(a_pCtx);
		}
	}


	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetCPUHandle(const std::string& a_name)
	{
		//auto _tex = Resource::TextureManager::Instance().GetTexture(a_name);
		auto _id = m_upRGResourceManager->GetID(a_name);
		auto _texHandle = m_upRGResourceManager->GetTexHandle(_id);
		const auto* _pTex = Resource::ResourceManager::Instance().Get(_texHandle);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_pTex->GetSRV());
	}

	Engine::Resource::Handle<Engine::Resource::Texture> RenderGraph::CreateTexture(
		const std::string& a_name,
		const DXGI_FORMAT& a_format,
		const UINT64& a_widht,
		const UINT& a_height,
		const Resource::TextureUsage& a_texUsage
	)
	{
		Resource::TextureCreateDesc _desc = {
			.name = a_name,
			.width = a_widht,
			.height = a_height,
			.format = a_format,
			.usage = a_texUsage
		};
		//return Resource::TextureManager::Instance().CreateTexture(_desc);
		return Resource::TextureLoader::Create(_desc);
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

	Resource::ID RenderGraph::GetID(const std::string& a_resourceName)
	{
		return m_upRGResourceManager->GetID(a_resourceName);
	}

	Resource::Handle<Resource::Texture> RenderGraph::GetTexHandle(const std::string& a_resourceName)
	{
		auto _id = GetID(a_resourceName);
		return m_upRGResourceManager->GetTexHandle(_id);
	}

	std::vector<std::string> RenderGraph::GetRGResourceList()
	{
		return m_upRGResourceManager->GetResourceNameVec();
	}


	DXGI_FORMAT RenderGraph::GetDXGIFormat(Resource::ID a_id)
	{
		return m_upRGResourceManager->GetDXGIFormat(a_id);
	}

	void RenderGraph::AutoBarrier(RenderContext* a_pCtx,CompiledPass& a_pass)
	{
		// 使用するリソースバリア
		for (auto& _barrier : a_pass.barrierVec)
		{
			// 現在のステートと変更予定ステートが違うのならば変更する
			if (m_upRGResourceManager->RefCurrentState(_barrier.resID) != _barrier.after)
			{
				// ステート変更
				a_pCtx->Transition(
					//Resource::TextureManager::Instance().RefTexture(_barrier.texHandle).GetResource(),
					Resource::ResourceManager::Instance().Ref(_barrier.texHandle)->GetResource(),
					m_upRGResourceManager->RefCurrentState(_barrier.resID),
					_barrier.after
				);
				// 現在のステートを更新
				m_upRGResourceManager->RefCurrentState(_barrier.resID) = _barrier.after;
			}
		}
	}

}