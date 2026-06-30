#include "ParticlePass.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "../../../../MainEngine.h"
#include "../../../../Particle/ParticleBufferManager.h"
#include "../../../../Particle/GPU/GPUParticlePool/GPUParticlePool.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "../../../../Option/OptionManager.h"

#include "../../../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Graphics
{
	void AddParticlePass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "ParticleDraw";
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/ParticleShader/ParticleVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/ParticleShader/ParticleVS.cso");

		// 依存関係構築
		_rpBuilder.ReadDepth("Depth");
		_rpBuilder.WriteRTV("AffterLighting", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Load, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("ParticleDraw", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/ParticleShader/ParticleVS.cso", D3D12::Input::gParticleInputLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/ParticleShader/ParticlePS.cso");

		// PSOブレンド設定
		 //カラーブレンド
		_sPso.BlendEnable(true);
		_sPso.SrcBlend(D3D12_BLEND_SRC_ALPHA, 0);
		_sPso.DestBlend(D3D12_BLEND_ONE, 0);
		_sPso.BlendOp(D3D12_BLEND_OP_ADD, 0);

		// アルファブレンド
		_sPso.SrcBlendAlpha(D3D12_BLEND_ONE, 0);
		_sPso.DestBlendAlpha(D3D12_BLEND_ZERO, 0);
		_sPso.BlendOpAlpha(D3D12_BLEND_OP_ADD, 0);
		_sPso.desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// 深度設定
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
		
		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				auto* _particleManager = MainEngine::Instance().RefParticleManager();
				// 全プール分回す
				for (auto& [_handle, _pool] : _particleManager->GetPoolMap())
				{
					// プールが読み込み済みかチェック
					if (!_particleManager->IsLoaded(_handle)) continue;

					// ヒープ、ルートシグネチャバインド
					a_pCtx->BindHeap();
					a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);
					a_pGE->BindPSO(a_pCtx,_spPassData->staticIndex);
					// カメラバインド
					CameraData _cbCam = a_pGE->GetCameraData();
					a_pCtx->GraphicsBindRootCBV(0, _cbCam);

					// パーティクルデータバインド
					auto _particleSRV = _pool->GetParticlePoolSRV();
					a_pCtx->BindSRV(1, _particleSRV);

					// パーティクル画像バインド
					auto* _pParticle = Resource::ResourceManager::Instance().Get(_handle);
					if (!_pParticle) continue;
					auto* _pTex = Resource::ResourceManager::Instance().Get(_pParticle->GetTexHandle());
					if (!_pTex) continue;
					a_pCtx->BindSRV(2,_pTex->GetSRV());

					// 描画
					a_pCtx->DrawPolygonInstancing(_pool->GetMaxCapacity());
					ENGINE_LOG("Particle描画中 : %s",_pParticle->GetName().c_str());
				}
			};

		// パス登録
		a_rg.AddPassNode(a_phase, _node);
	}
	void AddParticlePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "ParticleDraw";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/ParticleShader/ParticleVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/ParticleShader/ParticleVS.cso");

		// 依存関係構築
		_rpBuilder.ReadDepth("Depth");
		_rpBuilder.WriteRTV("AffterLighting", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Load, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("ParticleDraw", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/ParticleShader/ParticleVS.cso", D3D12::Input::gParticleInputLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/ParticleShader/ParticlePS.cso");

		// PSOブレンド設定
		 //カラーブレンド
		_sPso.BlendEnable(true);
		_sPso.SrcBlend(D3D12_BLEND_SRC_ALPHA, 0);
		_sPso.DestBlend(D3D12_BLEND_ONE, 0);
		_sPso.BlendOp(D3D12_BLEND_OP_ADD, 0);

		// アルファブレンド
		_sPso.SrcBlendAlpha(D3D12_BLEND_ONE, 0);
		_sPso.DestBlendAlpha(D3D12_BLEND_ZERO, 0);
		_sPso.BlendOpAlpha(D3D12_BLEND_OP_ADD, 0);
		_sPso.desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// 深度設定
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				auto* _particleManager = MainEngine::Instance().RefParticleManager();
				// 全プール分回す
				for (auto& [_handle, _pool] : _particleManager->GetPoolMap())
				{
					// プールが読み込み済みかチェック
					if (!_particleManager->IsLoaded(_handle)) continue;

					// ヒープ、ルートシグネチャバインド
					a_pCtx->BindHeap();
					a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);
					a_pGE->BindPSO(a_pCtx, _spPassData->staticIndex);
					// カメラバインド
					CameraData _cbCam = a_pGE->GetCameraData();
					a_pCtx->GraphicsBindRootCBV(0, _cbCam);

					// パーティクルデータバインド
					auto _particleSRV = _pool->GetParticlePoolSRV();
					a_pCtx->BindSRV(1, _particleSRV);

					// パーティクル画像バインド
					auto* _pParticle = Resource::ResourceManager::Instance().Get(_handle);
					if (!_pParticle) continue;
					auto* _pTex = Resource::ResourceManager::Instance().Get(_pParticle->GetTexHandle());
					if (!_pTex) continue;
					a_pCtx->BindSRV(2, _pTex->GetSRV());

					// 描画
					a_pCtx->DrawPolygonInstancing(_pool->GetMaxCapacity());
					ENGINE_LOG("Particle描画中 : %s", _pParticle->GetName().c_str());
				}
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
