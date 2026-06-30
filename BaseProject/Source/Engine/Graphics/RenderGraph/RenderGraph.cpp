#include "RenderGraph.h"

// パス関連
#include "../RenderPass/RasterizePass/ZPrePass/ZPrePass.h"
#include "../RenderPass/RasterizePass/GBufferPass/GBufferPass.h"
#include "../RenderPass/ComputePass/Lighting/DeferredLighting/DeferredLighting.h"
#include "../RenderPass/RasterizePass/FullScreenPass/FullScreenPass.h"
#include "../RenderPass/RaytracingPass/RaytracingShadowPass/RaytracingShadowPass.h"
#include "../RenderPass/RaytracingPass/RaytracingGIPass/RaytracingGIPass.h"
#include "../RenderPass/RaytracingPass/FullRaytracingPass/FullRaytracingPass.h"
#include "../RenderPass/CopyPass/GBufferHistoryPass/GBufferHistoryPass.h"
#include "../RenderPass/CopyPass/PostHistoryPass/PostHistoryPass.h"
#include "../RenderPass/ComputePass/Denoise/TempralAccumulationPass/TemporalAccumulationPass.h"
#include "../RenderPass/ComputePass/Denoise/GI/GISpatialDenoisePass/GISpatialDenoisePass.h"
#include "../RenderPass/ComputePass/AntiAliasing/TAA/TAAPass.h"
#include "../RenderPass/ComputePass/Denoise/Shadow/ShadowTemporalAccumulationPass/ShadowTemporalAccumulationPass.h"
#include "../RenderPass/RasterizePass/ParticlePass/ParticlePass.h"
#include "../RenderPass/ComputePass/Effect/Particle/EmitParticlePass/EmitParticlePass.h"
#include "../RenderPass/ComputePass/Effect/Particle/UpdateParticlePass.h"
#include "../RenderPass/RasterizePass/DebugLinePass/DebugLinePass.h"

// マネージャー関連
#include "RGVarsionManager/RGResourceManager.h"
#include "../GraphicEngine.h"
#include "../RenderContext/RenderContext.h"
#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Resource/Loader/Texture/TextureLoader.h"

// オプション
#include "../../Option/OptionManager.h"

namespace Engine::Graphics
{
	RenderGraph::RenderGraph()
	{}
	RenderGraph::~RenderGraph()
	{}

	void RenderGraph::Init(D3D12::PipelineStateManager* a_pPipelineStateManager)
	{
		// オプション取得
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		UINT64 _winWidth = _winOp.windowWidth;
		UINT _winHeight = _winOp.windowHegiht;


		//// リソースマネージャー作成
		m_upRGResourceManager = std::make_unique<RGResourceManager>();
		//m_upRGResourceManager->Register(
		//	"MainColor",
		//	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"QuadTexture",
		//	DXGI_FORMAT_R16G16B16A16_FLOAT,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"UITexture",
		//	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV,
		//	{0,0,0,0}
		//);
		//m_upRGResourceManager->Register(
		//	"GBufferAlbedo",
		//	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"GBufferNormal",
		//	DXGI_FORMAT_R16G16_FLOAT,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"GBufferMaterial",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"GBufferEmissiv",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"Depth",
		//	DXGI_FORMAT_R32_TYPELESS,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::DSV | Resource::TextureUsage::SRV
		//);
		//m_upRGResourceManager->Register(
		//	"PrevDepth",
		//	DXGI_FORMAT_R32_TYPELESS,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::DSV | Resource::TextureUsage::SRV
		//);
		//m_upRGResourceManager->Register(
		//	"PrevNormal",
		//	DXGI_FORMAT_R16G16_FLOAT,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"GBufferVelocity",
		//	DXGI_FORMAT_R16G16_FLOAT,
		//	_winWidth,
		//	_winHeight,
		//	Resource::TextureUsage::SRV | Resource::TextureUsage::RTV
		//);
		//// レイの結果用
		//m_upRGResourceManager->Register(
		//	"RayShadow",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"RayGI",
		//	DXGI_FORMAT_R16G16B16A16_FLOAT,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"FullRay",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"Test",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"SpatialTemp_A",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"SpatialTemp_B",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->RegisterTemporal(
		//	"DenoiseGI",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->RegisterTemporal(
		//	"AffterDLShadowTempAccumu",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		// m_upRGResourceManager->Register(
		//	"ShadowHistory_A",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		// m_upRGResourceManager->Register(
		//	"ShadowHistory_B",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		// m_upRGResourceManager->Register(
		//	"AffterDLShadowTempAccumu",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"FinalGI",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"FinalGI",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"DeferedLighting_A",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"DeferedLighting_B",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
		//);
		//m_upRGResourceManager->Register(
		//	"AffterLighting",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"AffterTAAColor",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"HistoryTAAColor",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);
		//m_upRGResourceManager->Register(
		//	"AffterParticle",
		//	DXGI_FORMAT_R8G8B8A8_UNORM,
		//	_winWidth,
		//	_winHeight,
		//	Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::RTV
		//);

		// パス登録
		AddZPrePass(a_pPipelineStateManager, *this, EDrawPhase::Setup);
		
		AddGBufferPass(a_pPipelineStateManager, *this, EDrawPhase::Geometry);
		AddFullScreenPass(a_pPipelineStateManager, *this, EDrawPhase::Present);
		AddFullRaytracingPass(a_pPipelineStateManager, *this, EDrawPhase::Geometry);
		AddRaytracingShadowPass(a_pPipelineStateManager, *this, EDrawPhase::Shadow);
		AddRaytracingGIPass(a_pPipelineStateManager, *this, EDrawPhase::Raytracing);
		AddDeferredLighting(a_pPipelineStateManager, *this, EDrawPhase::Lighting);
		AddGBufferHistoryPass(a_pPipelineStateManager, *this, EDrawPhase::HistoryUpdate);
		AddPostHistoryPass(a_pPipelineStateManager, *this, EDrawPhase::HistoryUpdate);
		AddDebugLinePass(a_pPipelineStateManager, *this, EDrawPhase::UI);

		AddTAAPass(a_pPipelineStateManager, *this, EDrawPhase::PostProcess);
		

		// 自分で順序を決定するパス(登録準に配列に追加される)
		// シャドウデノイズ系
		AddShadowTemporalAccumulationPass(a_pPipelineStateManager, *this, EDrawPhase::NotSort);

		// GIデノイズ系
		AddTemporalAccumulationPass(a_pPipelineStateManager, *this, EDrawPhase::NotSort);
		AddGISpatialDenoisePass(a_pPipelineStateManager, *this, EDrawPhase::NotSort);

		// パーティクル
		AddEmitParticlePass(a_pPipelineStateManager, *this, EDrawPhase::Particle);
		AddUpdateParticlePass(a_pPipelineStateManager, *this, EDrawPhase::Particle);
		AddParticlePass(a_pPipelineStateManager, *this, EDrawPhase::Particle);

		// コンパイル
		Compile();
	}

	void RenderGraph::Release()
	{
		m_compiledPasses.clear();
		m_passNodeMap.clear();
	}

	void RenderGraph::Compile()
	{
		m_compiledPasses.clear();

		// リソースの作成
		CreateResource();

		// =========================================================
		// 文字列から Resource::ID を取得し、配列に流し込む
		for (auto& [_phase, _passNodeVec] : m_passNodeMap)
		{
			for (auto& _passNode : _passNodeVec)
			{
				_passNode.read.clear();
				_passNode.write.clear();
				_passNode.resourceAccessVec.clear();

				for (auto& _req : _passNode.readRequests)
				{
					Resource::ID _id = m_upRGResourceManager->GetID(_req.resName);
					if (_id == Resource::Limits::INVALID_ID)
					{
						assert(_id != Resource::Limits::INVALID_ID && "要求されたリソース名がレンダーグラフに登録されていません！タイポをチェックしてください。");
					}
					_passNode.read.push_back(_id);
					_passNode.resourceAccessVec.push_back({ _id, _req.type, _req.load, _req.store });
				}
				for (auto& _req : _passNode.writeRequests)
				{
					Resource::ID _id = m_upRGResourceManager->GetID(_req.resName);
					if (_id == Resource::Limits::INVALID_ID)
					{
						assert(_id != Resource::Limits::INVALID_ID && "要求されたリソース名がレンダーグラフに登録されていません！タイポをチェックしてください。");
					}
					_passNode.write.push_back(_id);
					_passNode.resourceAccessVec.push_back({ _id, _req.type, _req.load, _req.store });
				}
			}
		}
		

		// ソート配列の作成
		for (auto& [_phase, _passVec] : m_passNodeMap)
		{
			std::vector<RenderPassNode*> _sortedNodes = {};
			if (_phase == EDrawPhase::NotSort)
			{
				for (auto& _pass : _passVec)
				{
					_sortedNodes.push_back(&_pass);
				}
			}
			else if (_phase == EDrawPhase::Particle)
			{
				// 外部のバッファなどはまだバリア未対応なので
				// ソートしない
				for (auto& _pass : _passVec)
				{
					_sortedNodes.push_back(&_pass);
				}
			}
			else
			{
				// ソート配列の作成
				Algorithm::Graph::TopologicalSort(
					_passVec,
					_sortedNodes,
					[&](auto& a, auto& b)
					{
						// 依存があるかどうか
						for (auto& _write : b.write)
						{
							for (auto& _read : a.read)
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
			}

			// 合成
			m_sortedPassed.insert(m_sortedPassed.end(),_sortedNodes.begin(),_sortedNodes.end());
		}

		// テクスチャの作成
		m_upRGResourceManager->CreateAllTexture();

		// リソース実態の作成
		for(auto* _pass : m_sortedPassed)
		{
			CompiledPass _cp;
			_cp.pNode = _pass;

			// リソース遷移作成
			for (auto _access : _pass->resourceAccessVec)
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
					if (_access.type == AccessType::Depth_Read)
					{
						_cp.dsvHandle = m_upRGResourceManager->GetReadOnlyDSVHandle(_access.id);
					}
					else
					{
						_cp.dsvHandle = m_upRGResourceManager->GetDSVHandle(_access.id);
					}
					
					if (_access.load == LoadOp::Clear)
					{
						_cp.isDepthClear = true;
					}
				}

				// バリア作成
				D3D12_RESOURCE_STATES _next = D3D12_RESOURCE_STATE_COMMON;
				if (_access.type == AccessType::SRV)
				{
					_next = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
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
				if (_access.type == AccessType::CopySrc)
				{
					_next = D3D12_RESOURCE_STATE_COPY_SOURCE;
				}
				if (_access.type == AccessType::CopyDst)
				{
					_next = D3D12_RESOURCE_STATE_COPY_DEST;
				}

				bool _isRead = (_access.type == AccessType::SRV || _access.type == AccessType::Depth_Read);
				auto& _currentState = m_upRGResourceManager->RefCurrentState(_access.id,_isRead);
				if (_currentState != _next)
				{
					_cp.barrierVec.push_back(
						{
							m_upRGResourceManager->GetTexHandle(_access.id,_isRead),
							_currentState,
							_next,
							_access.id,
							_isRead
						}
					);
					_currentState = _next;
				}
			}

			// パスのインデックスを指定
			_cp.pNode->passIndex = static_cast<uint8_t>(m_compiledPasses.size());

			// 実行データに入れていく
			m_compiledPasses.push_back(_cp);
		}
		
		m_upRGResourceManager->StateReset();
	}

	void RenderGraph::Excute(GraphicsEngine* a_pGE,RenderContext* a_pCtx)
	{
		// テンポラルスワップ処理
		m_upRGResourceManager->Swap();
		Swap();

		// コンパイル済みパスを順次実行していく
		for (auto& _cp : m_compiledPasses)
		{
			Editor::MainEditor::Instance().StartWatch(_cp.pNode->name);
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
			if (_cp.pNode && _cp.pNode->executeFunc)
			{
				_cp.pNode->executeFunc(a_pGE, a_pCtx, _cp.pNode->passIndex);
			}
			Editor::MainEditor::Instance().EndWatch(_cp.pNode->name);
		}
	}

	void RenderGraph::AddPassNode(const EDrawPhase& a_pahse, const RenderPassNode& a_node)
	{
		m_passNodeMap[a_pahse].push_back(a_node);
	}


	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetCPUHandle(const std::string& a_name)
	{
		//auto _tex = Resource::TextureManager::Instance().GetTexture(a_name);
		auto _id = m_upRGResourceManager->GetID(a_name);
		auto _texHandle = m_upRGResourceManager->GetTexHandle(_id);
		const auto* _pTex = Resource::ResourceManager::Instance().Get(_texHandle);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_pTex->GetSRV());
	}

	Handle<D3D12::SRV> RenderGraph::GetSRVHandle(const std::string& a_name)
	{
		auto _id = m_upRGResourceManager->GetID(a_name);
		auto _texHandle = m_upRGResourceManager->GetTexHandle(_id,true);
		const auto* _pTex = Resource::ResourceManager::Instance().Get(_texHandle);
		return _pTex->GetSRV();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetSRVCPU(const std::string& a_name)
	{
		auto _srvHandle = GetSRVHandle(a_name);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_srvHandle);
	}

	Handle<D3D12::UAV> RenderGraph::GetUAVHandle(const std::string& a_name, bool a_read)
	{
		auto _id = m_upRGResourceManager->GetID(a_name);
		auto _texHandle = m_upRGResourceManager->GetTexHandle(_id,a_read);
		const auto* _pTex = Resource::ResourceManager::Instance().Get(_texHandle);
		return _pTex->GetUAV();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::GetUAVCPU(const std::string& a_name, bool a_read)
	{
		auto _uavHandle = GetUAVHandle(a_name,a_read);
		return D3D12::DescriptorHeapManager::Instance().GetCPU(_uavHandle);
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
		case AccessType::CopySrc:
			return m_upRGResourceManager->Read(a_resourceName, Resource::TextureUsage::None);
		case AccessType::CopyDst:
			return m_upRGResourceManager->Read(a_resourceName, Resource::TextureUsage::None);
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
		case AccessType::CopySrc:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::None);
		case AccessType::CopyDst:
			return m_upRGResourceManager->Write(a_resourceName, Resource::TextureUsage::None);
		default:
			break;
		}
		return Resource::ID();
	}

	Resource::ID RenderGraph::GetID(const std::string& a_resourceName)
	{
		return m_upRGResourceManager->GetID(a_resourceName);
	}

	Handle<Resource::Texture> RenderGraph::GetTexHandle(const std::string& a_resourceName)
	{
		auto _id = GetID(a_resourceName);
		return m_upRGResourceManager->GetTexHandle(_id);
	}

	uint8_t RenderGraph::GetPassIndex(const std::string& a_passName)
	{
		if (m_compiledPasses.empty()) return 255;

		for (auto& _pass : m_compiledPasses)
		{
			if (_pass.pNode->name == a_passName)
			{
				return _pass.pNode->passIndex;
			}
		}

		return 255;
	}

	const RenderPassNode* RenderGraph::GetPass(const std::string& a_passName)
	{
		if (m_compiledPasses.empty()) return nullptr;

		for (auto& _pass : m_compiledPasses)
		{
			if (_pass.pNode->name == a_passName)
			{
				return _pass.pNode;
			}
		}

		return nullptr;
	}


	std::vector<std::string> RenderGraph::GetRGResourceList()
	{
		return m_upRGResourceManager->GetResourceNameVec();
	}

	UINT RenderGraph::GetTemporalIndex() const
	{
		return m_temporalIndex;
	}

	const RGResourceManager* RenderGraph::GetRGResourceManager() const
	{
		return m_upRGResourceManager.get();
	}

	RGResourceManager* RenderGraph::RefRGResourceManager()
	{
		return m_upRGResourceManager.get();
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
			if (m_upRGResourceManager->RefCurrentState(_barrier.resID,_barrier.isRead) != _barrier.after)
			{
				auto _currentTexHandle = m_upRGResourceManager->GetTexHandle(_barrier.resID,_barrier.isRead);

				// ステート変更
				a_pCtx->Transition(
					//Resource::ResourceManager::Instance().Ref(_barrier.texHandle)->GetResource(),
					Resource::ResourceManager::Instance().Ref(_currentTexHandle)->GetResource(),
					m_upRGResourceManager->RefCurrentState(_barrier.resID, _barrier.isRead),
					_barrier.after
				);
				// 現在のステートを更新
				m_upRGResourceManager->RefCurrentState(_barrier.resID, _barrier.isRead) = _barrier.after;
			}
		}
	}

	void RenderGraph::Swap()
	{
		m_temporalIndex = 1 - m_temporalIndex;
	}

	void RenderGraph::CreateResource()
	{
		auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		// =========================================================
		// 全パスからリソース要求を収集し、Usageをグローバルに合成する
		struct GlobalResourceInfo {
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			Resource::TextureUsage usage = Resource::TextureUsage::None;
			float texScale = 1.0f;
			bool isTemporal = false;
		};
		std::unordered_map<std::string, GlobalResourceInfo> _globalResourceMap = {};

		for (auto& [_phase, _passNodeVec] : m_passNodeMap)
		{
			for (auto& _passNode : _passNodeVec)
			{
				// Read要求の収集
				for (auto& _req : _passNode.readRequests)
				{
					auto& _info = _globalResourceMap[_req.resName];
					if (_req.type == AccessType::Depth_Read) _info.usage |= Resource::TextureUsage::DSV;
					if (_req.type == AccessType::SRV)        _info.usage |= Resource::TextureUsage::SRV;
					if (_req.type == AccessType::UAV)        _info.usage |= Resource::TextureUsage::UAV;

					if (_req.isTemporal) _info.isTemporal = true;
				}

				// Write要求の収集（フォーマットとスケールはWrite側が主導権を持つ）
				for (auto& _req : _passNode.writeRequests)
				{
					auto& _info = _globalResourceMap[_req.resName];
					if (_req.isTemporal) _info.isTemporal = true;

					if (_req.format != DXGI_FORMAT_UNKNOWN) _info.format = _req.format;
					_info.texScale = _req.texScale; // 念のためスケールも更新
					
					if (_req.type == AccessType::Depth_Write) _info.usage |= Resource::TextureUsage::DSV;
					if (_req.type == AccessType::RTV)         _info.usage |= Resource::TextureUsage::RTV;
					if (_req.type == AccessType::UAV)         _info.usage |= Resource::TextureUsage::UAV;

				}
			}
		}
		// =========================================================
		// 合成された情報をもとに、実際にリソースを Register する
		for (const auto& [_resName, _info] : _globalResourceMap)
		{
			// 外部から渡されるバッファ（スワップチェーン等）はフォーマット指定がないのでスキップ
			if (_info.format != DXGI_FORMAT_UNKNOWN)
			{
				m_upRGResourceManager->Register(
					_resName,
					_info.format,
					_winOp.windowWidth * _info.texScale,
					_winOp.windowHegiht * _info.texScale,
					_info.usage, 
					_info.isTemporal
				);
			}
		}
	}

}