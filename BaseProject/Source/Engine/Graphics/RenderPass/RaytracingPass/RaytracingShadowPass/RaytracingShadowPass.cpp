#include "RaytracingShadowPass.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"


namespace Engine::Graphics
{
	void RaytracingShadowPass::Excute(RenderContext* a_pCtx)
	{
		auto _texHandle = m_pRG->GetTexHandle("RayShadow");
		//Engine::Raytracing::RayEngine::Instance().Dispatch(_texHandle, a_pCtx, &m_rayPSO, &m_shaderTable);
		Engine::Raytracing::RayEngine::Instance().Dispatch(a_pCtx);
	}
	void RaytracingShadowPass::CreatePass()
	{
		// PSOの作成
		Raytracing::RayPSODesc _psoInit = {};
		_psoInit.shaderPass = "Asset/Shader/Ray/Raytracing.hlsl";
		_psoInit.AddShader(L"RayGen", Raytracing::LocalRootSignature::RayGen, Raytracing::ShaderCategory::RayGenerator);
		_psoInit.AddShader(L"Miss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddShader(L"ClosestHit", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowCHS", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowMiss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddHitGroup(L"HitGroup", L"ClosestHit");
		_psoInit.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS");
		_psoInit.maxRecursionDepth = 4;
		_psoInit.pGlobalRootSig = m_pRootSigMana->Ref("RayGlobal");
		_psoInit.pHitRootSig = m_pRootSigMana->Ref("Hit");
		_psoInit.pRayGenRootSig = m_pRootSigMana->Ref("RayGen");
		_psoInit.pMissRootSig = m_pRootSigMana->Ref("Miss");

		m_rayPSO.Init(_psoInit);

		// シェーダーテーブルの作成
		Raytracing::ShaderTableInit _shaderTableInit = {
			.pRayPSO = &m_rayPSO,
			.shaderData = _psoInit.shaderDataVec,
			.hitGroup = _psoInit.hitGroupVec,
			.maxInstance = 1000,
			.maxLocalRootSize = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * 3
		};
		m_shaderTable.Init(_shaderTableInit);

		//AddWrite("RayShadow", AccessType::UAV, LoadOp::Clear, StoreOp::Store);
	}
}