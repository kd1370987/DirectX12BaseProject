#include "GroundVolumetricFogPass.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Pipeline
{
	void GroundVolumetricFogPass::SetupSlots()
	{
		// 地面だけの深度(GroundDepthPass の出力) : レイの終点を戻す。
		// フォグの濃さは衝撃の配列から1歩ごとに求めるので、GroundFieldPass の出力は使わない
		DeclareInput("GroundDepth", EAccessType::SRV, EPassSlotType::Texture, true, kRootInputSRV);

		// フォグ。rgb = 色 / a = 濃さ。
		// 全画素を書き潰すのでクリアは不要
		DeclareOutput("Fog", "GroundFog", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, kRootOutputUAV);
	}

	void GroundVolumetricFogPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/GroundEffect/GroundVolumetricFogCS.cso", "GroundVolumetricFogCS");
	}

	void GroundVolumetricFogPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList || !a_context.pResourceManager) return;

		auto* _pCmd = a_context.pCmdList;
		auto& _resManager = *a_context.pResourceManager;

		// カメラ : 深度からワールド座標を戻す
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
			_pCmd, kRootCameraCB, _pGE->GetSceneView()->GetCameraData());

		// 調整値 : 経過時間と衝撃の数はパスが詰める
		m_elapsedTime += MainEngine::Instance().GetDeltaTime();

		GroundFogCB _cb = m_params.cb;
		_cb.time = m_elapsedTime;
		_cb.impulseCount = _pGE->GetGroundImpulseCount();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, kRootFogCB, _cb);

		// 衝撃の配列 : GraphicsEngine が今フレームぶんを詰め直したもの
		const UINT _impulseIndex = _pGE->GetGroundImpulseBuffer().GetSRV().GetIndex();
		_pCtx->ComputeBindDescriptorIndices(kRootImpulseSRV, std::span<const UINT>(&_impulseIndex, 1));

		// ノイズテクスチャ : GUID が変わっていたら読み直す。
		// 読み込みは待たないので、届くまでのフレームはノイズなしで描く
		if (m_loadedNoiseGUID != m_params.noiseTexGUID)
		{
			m_loadedNoiseGUID = m_params.noiseTexGUID;
			m_noiseTexRef = m_params.noiseTexGUID.IsValid()
				? _resManager.RequestLoad<Resource::Texture>(m_params.noiseTexGUID)
				: ResourceRef<Resource::Texture>{};
		}

		UINT _noiseIndex = kNoiseIndexNone;
		if (m_noiseTexRef && _resManager.IsReady(m_noiseTexRef))
		{
			if (const auto* _pNoiseTex = _resManager.Get(m_noiseTexRef))
			{
				_noiseIndex = _pNoiseTex->GetSRV().GetIndex();
			}
		}
		_pCtx->ComputeBindDescriptorIndices(kRootNoiseSRV, std::span<const UINT>(&_noiseIndex, 1));

		DispatchFullScreen(a_context);
	}



	void GroundVolumetricFogPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("fogHeight", m_params.cb.fogHeight);
		a_arch.Field("density", m_params.cb.density);
		a_arch.Field("noiseScale", m_params.cb.noiseScale);
		a_arch.Field("fogColor", m_params.cb.fogColor);
		a_arch.Field("stepSize", m_params.cb.stepSize);
		a_arch.GUIDField("noiseTexGUID", m_params.noiseTexGUID);
	}
}
