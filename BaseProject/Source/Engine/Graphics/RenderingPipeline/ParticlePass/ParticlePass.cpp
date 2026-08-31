#include "ParticlePass.h"

#include "../../GraphicEngine.h"
#include "../../RenderContext/RenderContext.h"

#include "../../../MainEngine.h"
#include "../../../Particle/ParticleBufferManager.h"
#include "../../../Particle/GPU/GPUParticlePool/GPUParticlePool.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Pipeline
{
	void ParticlePass::SetupSlots()
	{
		// 深度は読むだけ : 半透明なので書かず、手前のものには隠される
		DeclareInput("Depth", EAccessType::Depth_Read);

		// 描き足す先 : 「前段が描いた絵の上に重ねる」という順序をこの線で表す
		DeclareInput("Color", EAccessType::RTV, EPassSlotType::Texture, false);

		// すでに描かれている絵へ重ねるので Load。
		// HDR : ライティングと同じフォーマットで揃える
		Slot& _color = DeclareOutput("Color", "AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::RTV);
		_color.loadOp = ELoadOp::Load;
	}

	// 描き足す先が繋がっていれば、そのリソースへ重ねる
	void ParticlePass::OnLinksResolved()
	{
		FollowInputToOutput("Color", "Color");
	}

	void ParticlePass::Compile(const PassContext& a_context)
	{
		// 深度は共通 : 半透明なので深度は書かず、手前のものには隠される
		auto _setupCommon = [](D3D12::GraphicsPipelineDesc& a_pso)
			{
				a_pso.desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

				a_pso.DepthEnable(true);
				a_pso.DepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
				a_pso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
			};

		const std::string _vsPath = "Asset/Shader/Source/Particle/Draw/ParticleVS.cso";
		const std::string _psPath = "Asset/Shader/Source/Particle/Draw/ParticlePS.cso";

		// ---- 加算合成 : 光り物。重ねるほど明るくなり、描く順番に依存しない ----
		SetupRasterShader(
			a_context, _vsPath, _psPath, D3D12::Input::gParticleInputLayout,
			"ParticleDraw_Additive",
			[&_setupCommon](D3D12::GraphicsPipelineDesc& a_pso)
			{
				a_pso.BlendEnable(true);
				a_pso.SrcBlend(D3D12_BLEND_SRC_ALPHA, 0);
				a_pso.DestBlend(D3D12_BLEND_ONE, 0);
				a_pso.BlendOp(D3D12_BLEND_OP_ADD, 0);

				a_pso.SrcBlendAlpha(D3D12_BLEND_ONE, 0);
				a_pso.DestBlendAlpha(D3D12_BLEND_ZERO, 0);
				a_pso.BlendOpAlpha(D3D12_BLEND_OP_ADD, 0);

				_setupCommon(a_pso);
			},
			EPassHeapMode::Default,
			&m_additivePSO);

		// ---- 半透明合成 : 煙や破片。背景を明るくせず、前のものが後ろを隠す ----
		SetupRasterShader(
			a_context, _vsPath, _psPath, D3D12::Input::gParticleInputLayout,
			"ParticleDraw_AlphaBlend",
			[&_setupCommon](D3D12::GraphicsPipelineDesc& a_pso)
			{
				a_pso.BlendEnable(true);
				a_pso.SrcBlend(D3D12_BLEND_SRC_ALPHA, 0);
				a_pso.DestBlend(D3D12_BLEND_INV_SRC_ALPHA, 0);
				a_pso.BlendOp(D3D12_BLEND_OP_ADD, 0);

				a_pso.SrcBlendAlpha(D3D12_BLEND_ONE, 0);
				a_pso.DestBlendAlpha(D3D12_BLEND_INV_SRC_ALPHA, 0);
				a_pso.BlendOpAlpha(D3D12_BLEND_OP_ADD, 0);

				_setupCommon(a_pso);
			},
			EPassHeapMode::Default,
			&m_alphaBlendPSO);
	}

	void ParticlePass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE) return;

		auto* _particleManager = MainEngine::Instance().RefParticleManager();
		if (!_particleManager) return;

		//----------------------------------------------------------
		// 1アセット分を描く
		//----------------------------------------------------------
		auto _drawPool = [&](
			const Handle<Resource::ParticlesAsset>& a_handle,
			const auto& a_upPool,
			const Resource::ParticlesAsset& a_particle,
			const Handle<ID3D12PipelineState>& a_psoHandle)
			{
				// ヒープとルートシグネチャはグラフが張ってあるので、PSOだけ選び直す
				_pGE->BindPSO(_pCtx, a_psoHandle);

				// カメラバインド
				CameraData _cbCam = _pGE->GetCameraData();
				_pCtx->GraphicsBindRootCBV(0, _cbCam);

				// パーティクルデータバインド
				auto _particleSRV = a_upPool->GetParticlePoolSRV();
				_pCtx->BindSRV(1, _particleSRV);

				// パーティクル画像バインド
				auto* _pTex = Resource::ResourceManager::Instance().Get(a_particle.GetTexHandle());
				if (!_pTex) return;
				_pCtx->BindSRV(2, _pTex->GetSRV());

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

				_pCtx->GraphicsBindRootCBV(3, _cbDraw);

				// 描画
				_pCtx->DrawPolygonInstancing(a_upPool->GetMaxCapacity());
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
			const Handle<ID3D12PipelineState>& _psoHandle = (_mode == Particle::EParticleBlendMode::Additive)
				? m_additivePSO
				: m_alphaBlendPSO;

			if (!_psoHandle.IsValid()) continue;

			for (auto& [_handle, _pool] : _particleManager->GetPoolMap())
			{
				// プールが読み込み済みかチェック
				if (!_particleManager->IsLoaded(_handle)) continue;

				auto* _pParticle = Resource::ResourceManager::Instance().Get(_handle);
				if (!_pParticle) continue;
				if (_pParticle->GetBlendMode() != _mode) continue;

				_drawPool(_handle, _pool, *_pParticle, _psoHandle);
			}
		}
	}

	EPassEditResult ParticlePass::EditUpdate()
	{
		ImGui::TextDisabled("発生と更新は GraphicsEngine 側で毎フレーム1回走ります");
		ImGui::TextDisabled("このパスは描画だけを担当します");
		return EPassEditResult::None;
	}

	void ParticlePass::EditNode()
	{}

	void ParticlePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
