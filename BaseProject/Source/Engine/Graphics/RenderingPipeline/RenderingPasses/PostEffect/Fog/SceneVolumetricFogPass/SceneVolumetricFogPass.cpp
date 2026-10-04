#include "SceneVolumetricFogPass.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Pipeline
{
	void SceneVolumetricFogPass::SetupSlots()
	{
		// 同じ番号を指定した入力は、宣言した順にルート定数へ並ぶ(シェーダーの b100 と合わせる)。
		// シーン全体の深度 : レイの終点
		DeclareInput("SceneDepth", EAccessType::SRV, EPassSlotType::Texture, true, kRootInputSRV);
		// 地面だけの深度(GroundDepthPass の出力。任意) : ダストの高さの基準。
		// 繋がないとダストは出ない
		DeclareInput("GroundDepth", EAccessType::SRV, EPassSlotType::Texture, false, kRootInputSRV);
		// グラウンドフィールド(GroundFieldPass の出力。任意) : 衝撃で払われた・寄せられたチリ。
		// 繋がないとチリは衝撃で動かない
		DeclareInput("GroundField", EAccessType::SRV, EPassSlotType::Texture, false, kRootInputSRV);

		// フォグ。rgb = 色 / a = 濃さ。
		// 全画素を書き潰すのでクリアは不要
		DeclareOutput("Fog", "SceneFog", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, kRootOutputUAV);
	}

	void SceneVolumetricFogPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Fog/SceneVolumetricFogCS.cso", "SceneVolumetricFogCS");
	}

	void SceneVolumetricFogPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList || !a_context.pResourceManager) return;

		auto* _pCmd = a_context.pCmdList;
		auto& _resManager = *a_context.pResourceManager;

		// カメラ : 深度からワールド座標を戻す
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
			_pCmd, kRootCameraCB, _pGE->GetSceneView()->GetCameraData());

		// シーンのフォグ
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, kRootSceneFogCB, m_params.sceneFog);

		// グラウンドダスト : 経過時間だけはパスが進める
		m_elapsedTime += MainEngine::Instance().GetDeltaTime();

		GroundDustCB _dustCB = m_params.groundDust;
		_dustCB.time = m_elapsedTime;
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, kRootGroundDustCB, _dustCB);

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



	void SceneVolumetricFogPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		// シーンのフォグ
		a_arch.Field("sceneFogColor", m_params.sceneFog.fogColor);
		a_arch.Field("sceneFogDensity", m_params.sceneFog.density);
		a_arch.Field("sceneFogMaxDistance", m_params.sceneFog.maxDistance);

		// グラウンドダスト
		a_arch.Field("dustColor", m_params.groundDust.dustColor);
		a_arch.Field("dustDensity", m_params.groundDust.density);
		a_arch.Field("dustHeight", m_params.groundDust.height);
		a_arch.Field("dustNoiseScale", m_params.groundDust.noiseScale);
		a_arch.Field("dustStepSize", m_params.groundDust.stepSize);
		a_arch.GUIDField("noiseTexGUID", m_params.noiseTexGUID);
	}
}
