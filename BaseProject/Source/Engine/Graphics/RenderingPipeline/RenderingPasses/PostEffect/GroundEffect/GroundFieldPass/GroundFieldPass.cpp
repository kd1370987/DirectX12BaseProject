#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/GroundEffect/GroundFieldPass/GroundFieldPass.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Graphics/Frame/SceneView/CameraData.h"

namespace Engine::Graphics::Pipeline
{
	namespace
	{
		// グラウンドフィールドの定数
		// StructuredBuffer は要素数を持たないので、衝撃の数もここで渡す
		// ※ HLSL 側(Asset/Shader/Common/RootParameters/GroundFieldData.hlsli)と並びを合わせること
		struct GroundFieldCB
		{
			float time;				// パスが回り始めてからの経過時間(秒)
			float deltaTime;		// 前フレームからの経過時間(秒)
			uint32_t impulseCount;	// 今フレームの衝撃の数
			float pad0;
		};
	}

	void GroundFieldPass::SetupSlots()
	{
		// グラウンドフィールド。カメラを中心にした GROUND_FIELD_WORLD_SIZE (m) 四方を真上から並べたもの。
		//   r = 払われずに残ったチリの量 / g = 波頭に寄せられたチリの量
		// 画面とは関係のない広さなので、解像度は固定で持つ(描画解像度に追従させない)。
		// 全テクセルを書き潰すのでクリアは不要
		DeclareOutput("Field", "GroundField", DXGI_FORMAT_R16G16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, ROOT_OUTPUT_UAV,
			GROUND_FIELD_RESOLUTION, GROUND_FIELD_RESOLUTION);
	}

	void GroundFieldPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/GroundEffect/GroundFieldCS.cso", "GroundFieldCS");
	}

	void GroundFieldPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList) return;

		auto* _pCmd = a_context.pCmdList;

		// カメラ : フィールドの中心を決める
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
			_pCmd, ROOT_CAMERA_CB, _pGE->GetSceneView()->GetCameraData());

		// 経過時間と衝撃の数
		const float _deltaTime = MainEngine::Instance().GetDeltaTime();
		m_elapsedTime += _deltaTime;

		GroundFieldCB _cb = {};
		_cb.time = m_elapsedTime;
		_cb.deltaTime = _deltaTime;
		_cb.impulseCount = _pGE->GetGroundImpulseCount();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, ROOT_GROUND_FIELD_CB, _cb);

		// 衝撃の配列
		const UINT _impulseIndices[] = {
			_pGE->GetGroundImpulseBuffer().GetSRV().GetIndex(),
		};
		_pCtx->ComputeBindDescriptorIndices(ROOT_IMPULSE_SRV, _impulseIndices);

		// 画面ではなくフィールドの大きさで回す
		const Slot* _pOut = FindOutputSlot(MakeSlotID("Field"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}



	void GroundFieldPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
