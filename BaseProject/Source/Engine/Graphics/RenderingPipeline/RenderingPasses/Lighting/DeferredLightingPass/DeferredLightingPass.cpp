#include "DeferredLightingPass.h"

#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Graphics::Pipeline
{
	void DeferredLightingPass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=アンビエントCB / 2=SRVテーブル / 3=UAV
		//                    4=ライティング調整値CB / 5=ライト配列SRV / 6=ライト数CB
		// テーブルの並びはシェーダーの t0.. と同じ順にすること
		DeclareInput("Albedo", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Material", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Emissive", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Shadow", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("GI", EAccessType::SRV, EPassSlotType::Texture, true, 2);

		Slot& _out = DeclareOutput("Color", "AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 3);
		_out.loadOp = ELoadOp::Clear;
	}

	void DeferredLightingPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context,
			"Asset/Shader/Source/Lighting/Deferred/DeferredLightingShader.cso",
			"DeferredLightingShader");
	}

	void DeferredLightingPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList) return;

		auto* _pCmd = a_context.pCmdList;

		// カメラ
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(_pCmd, 0, _pGE->GetCameraData());

		// アンビエントカラー
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(_pCmd, 1, _pGE->GetAmbientData());

		// ライティング調整値。
		// もとは OptionManager から引いていたが、パイプラインごとに変えられるよう
		// このパスの持ち物にしてある
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 4, m_cb);

		// ライト
		//
		// ライト配列はグラフの資源ではないので、スロットの宣言には乗らない。
		// GraphicsEngine が Execute() で今フレームぶんを詰め直したものを、ここで直接張る。
		// テーブルは並べた順にディスクリプタが入るので、t7 = ポイント / t8 = 平行光 の順を崩さないこと
		const auto& _frameLight = _pGE->GetFrameLightData();
		const D3D12_CPU_DESCRIPTOR_HANDLE _lightSrvArr[] = {
			D3D12::DescriptorHeapManager::Instance().GetCPU(_frameLight.plBuffer.GetSRV()),
			D3D12::DescriptorHeapManager::Instance().GetCPU(_frameLight.dlBuffer.GetSRV()),
		};
		_pCtx->ComputeBindSRV(5, _lightSrvArr);

		// ライト数
		// StructuredBuffer は要素数を持たないので、ループの上限をCBで渡す。
		// HLSL の LightCountData と同じレイアウトで詰める
		struct LightCountCB
		{
			uint32_t directionalNum;
			uint32_t pointNum;
			uint32_t pad[2];
		};
		LightCountCB _countCB = {};
		_countCB.directionalNum = _frameLight.dlCount;
		_countCB.pointNum       = _frameLight.plCount;
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 6, _countCB);

		DispatchFullScreen(a_context);
	}

	EPassEditResult DeferredLightingPass::EditUpdate()
	{
		bool _isEdit = false;

		_isEdit |= ImGui::DragFloat("GIIntensity", &m_cb.giIntensity, 0.01f, 0.0f);
		_isEdit |= ImGui::DragFloat("DirectionalIntensity", &m_cb.directionalIntensity, 0.01f, 0.0f);
		_isEdit |= ImGui::DragFloat("DielectricF0", &m_cb.dielectricF0, 0.001f, 0.0f, 1.0f);

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void DeferredLightingPass::EditNode()
	{}

	void DeferredLightingPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("giIntensity", m_cb.giIntensity);
		a_arch.Field("directionalIntensity", m_cb.directionalIntensity);
		a_arch.Field("dielectricF0", m_cb.dielectricF0);
	}
}
