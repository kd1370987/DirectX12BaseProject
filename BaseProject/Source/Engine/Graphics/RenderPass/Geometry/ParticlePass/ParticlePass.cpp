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
	void AddParticlePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			Handle<ID3D12RootSignature> rootSigHandle = {};

			// 色の重ね方はアセット単位で選べるので、ブレンドだけ違うPSOを2つ持つ。
			// ブレンドはPSOに焼き込まれるため、実行時に切り替えるにはこうするしかない
			uint8_t additiveIndex = 0;
			uint8_t alphaBlendIndex = 0;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "ParticleDraw";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// パス共通設定

		// 依存関係構築
		_rpBuilder.ReadDepth("Depth");
		// HDR : AfterLighting はライティングと同じR16Fで揃える(フォーマット不一致はグラフが破綻する)
		_rpBuilder.WriteRTV("AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Load, StoreOp::Store);

		//------------------------------------------------------------------
		// PSO構築
		//
		// シェーダーも深度も共通で、違うのはブレンドだけ。
		// ブレンドはPSOに焼き込まれるので、アセットごとに切り替えるには
		// あらかじめ2つ作っておいて描くときに選ぶしかない。
		//
		// ※ CreatePSODesc は内部の配列へ push_back して参照を返す。
		//    2つ目を作った時点で1つ目の参照は無効になるので、
		//    かならず「作る → その場で設定しきる」を1つずつ行うこと。
		//------------------------------------------------------------------
		ID3DBlob* _pBlob = nullptr;

		// 深度は共通 : 半透明なので深度は書かず、手前のものには隠される
		auto _setupCommon = [&](D3D12::GraphicsPipelineDesc& a_pso)
			{
				a_pso.desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

				a_pso.DepthEnable(true);
				a_pso.DepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
				a_pso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
			};

		// ---- 加算合成 : 光り物。重ねるほど明るくなり、描く順番に依存しない ----
		{
			auto& _pso = _rpBuilder.CreatePSODesc("ParticleDraw_Additive", _spPassData->additiveIndex);
			_pBlob = _rpBuilder.SetVS(_pso, "Asset/Shader/Source/Particle/Draw/ParticleVS.cso", D3D12::Input::gParticleInputLayout);
			_rpBuilder.SetPS(_pso, "Asset/Shader/Source/Particle/Draw/ParticlePS.cso");

			_pso.BlendEnable(true);
			_pso.SrcBlend(D3D12_BLEND_SRC_ALPHA, 0);
			_pso.DestBlend(D3D12_BLEND_ONE, 0);
			_pso.BlendOp(D3D12_BLEND_OP_ADD, 0);

			_pso.SrcBlendAlpha(D3D12_BLEND_ONE, 0);
			_pso.DestBlendAlpha(D3D12_BLEND_ZERO, 0);
			_pso.BlendOpAlpha(D3D12_BLEND_OP_ADD, 0);

			_setupCommon(_pso);
		}

		// ---- 半透明合成 : 煙や破片。背景を明るくせず、前のものが後ろを隠す ----
		{
			auto& _pso = _rpBuilder.CreatePSODesc("ParticleDraw_AlphaBlend", _spPassData->alphaBlendIndex);
			_rpBuilder.SetVS(_pso, "Asset/Shader/Source/Particle/Draw/ParticleVS.cso", D3D12::Input::gParticleInputLayout);
			_rpBuilder.SetPS(_pso, "Asset/Shader/Source/Particle/Draw/ParticlePS.cso");

			_pso.BlendEnable(true);
			_pso.SrcBlend(D3D12_BLEND_SRC_ALPHA, 0);
			_pso.DestBlend(D3D12_BLEND_INV_SRC_ALPHA, 0);
			_pso.BlendOp(D3D12_BLEND_OP_ADD, 0);

			_pso.SrcBlendAlpha(D3D12_BLEND_ONE, 0);
			_pso.DestBlendAlpha(D3D12_BLEND_INV_SRC_ALPHA, 0);
			_pso.BlendOpAlpha(D3D12_BLEND_OP_ADD, 0);

			_setupCommon(_pso);
		}

		_spPassData->rootSigHandle = _rpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				auto* _particleManager = MainEngine::Instance().RefParticleManager();

				//----------------------------------------------------------
				// 1アセット分を描く
				//----------------------------------------------------------
				auto _drawPool = [&](
					const Handle<Resource::ParticlesAsset>& a_handle,
					const auto& a_upPool,
					const Resource::ParticlesAsset& a_particle,
					uint8_t a_psoIndex)
					{
						// ヒープ、ルートシグネチャバインド
						a_pCtx->BindHeap();
						a_pCtx->SetGraphicsRootSignature(_spPassData->rootSigHandle);
						a_pGE->BindPSO(a_pCtx, a_psoIndex);

						// カメラバインド
						CameraData _cbCam = a_pGE->GetCameraData();
						a_pCtx->GraphicsBindRootCBV(0, _cbCam);

						// パーティクルデータバインド
						auto _particleSRV = a_upPool->GetParticlePoolSRV();
						a_pCtx->BindSRV(1, _particleSRV);

						// パーティクル画像バインド
						auto* _pTex = Resource::ResourceManager::Instance().Get(a_particle.GetTexHandle());
						if (!_pTex) return;
						a_pCtx->BindSRV(2, _pTex->GetSRV());

						// 描画設定バインド : 板ポリの向きと、寿命に沿った見た目の変化はアセット単位
						Particle::ParticleDrawData _cbDraw = {};
						_cbDraw.orientation  = static_cast<uint32_t>(a_particle.GetOrientation());
						_cbDraw.stretch      = a_particle.GetStretch();
						_cbDraw.endSizeScale = a_particle.GetEndSizeScale();
						_cbDraw.fadeInRatio  = a_particle.GetFadeInRatio();
						_cbDraw.fadeOutRatio = a_particle.GetFadeOutRatio();
						_cbDraw.startColor   = a_particle.GetStartColor();
						_cbDraw.endColor     = a_particle.GetEndColor();

						// ローカル空間で回した粒を戻すための行列。
						// 席 0 は単位行列なので、ワールド空間の粒は素通りする
						const auto _emitterMatrices = _particleManager->GetEmitterMatrices(a_handle);
						const size_t _matCount =
							(std::min)(_emitterMatrices.size(), Particle::PARTICLE_EMITTER_MAX);
						for (size_t _m = 0; _m < _matCount; ++_m)
						{
							_cbDraw.emitterMatrices[_m] = _emitterMatrices[_m];
						}

						a_pCtx->GraphicsBindRootCBV(3, _cbDraw);

						// 描画
						a_pCtx->DrawPolygonInstancing(a_upPool->GetMaxCapacity());
					};

				//----------------------------------------------------------
				// 半透明 → 加算 の順で回す
				//
				// どちらも深度を書かないので、後から描いたものが上に乗る。
				// 煙(半透明)を先に置いてから炎(加算)を足すと、爆発の芯が煙の手前で光る。
				// 逆にすると煙が炎を覆い隠してしまう。
				// (同じ重ね方どうしの前後は並べ替えていないので、そこまでは面倒を見ない)
				//----------------------------------------------------------
				const Particle::EParticleBlendMode _drawOrder[] =
				{
					Particle::EParticleBlendMode::AlphaBlend,
					Particle::EParticleBlendMode::Additive,
				};

				for (const auto _mode : _drawOrder)
				{
					const uint8_t _psoIndex = (_mode == Particle::EParticleBlendMode::Additive)
						? _spPassData->additiveIndex
						: _spPassData->alphaBlendIndex;

					for (auto& [_handle, _pool] : _particleManager->GetPoolMap())
					{
						// プールが読み込み済みかチェック
						if (!_particleManager->IsLoaded(_handle)) continue;

						auto* _pParticle = Resource::ResourceManager::Instance().Get(_handle);
						if (!_pParticle) continue;
						if (_pParticle->GetBlendMode() != _mode) continue;

						_drawPool(_handle, _pool, *_pParticle, _psoIndex);
					}
				}
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
