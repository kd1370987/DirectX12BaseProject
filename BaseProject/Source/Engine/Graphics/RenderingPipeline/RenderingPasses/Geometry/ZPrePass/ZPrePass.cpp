#include "ZPrePass.h"

#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Pipeline
{
	void ZPrePass::SetupSlots()
	{
		// 不透明のモデルを受け取る
		m_geometryQueue = EGeometryQueue::Opaque;

		// 深度だけを書く。あとで SRV としても読めるように TYPELESS で確保する
		Slot& _depth = DeclareOutput(
			"Depth", "SceneDepth", DXGI_FORMAT_R32_TYPELESS, EAccessType::Depth_Write);
		_depth.loadOp = ELoadOp::Clear;
	}

	void ZPrePass::Compile(const PassContext& a_context)
	{
		if (!a_context.pGraphicsEngine) return;

		auto* _pPSOManager = a_context.pGraphicsEngine->RefPipelineStateManager();
		if (!_pPSOManager) return;

		auto& _assetDB = Resource::AssetDatabase::Instance();
		auto& _resManager = Resource::ResourceManager::Instance();

		const auto _guidMS = _assetDB.GetGUIDFromFilePath("Asset/Shader/Source/Geometry/MeshShader/UberMS.cso");
		const auto _msHandle = _resManager.LoadImmediate<Resource::Shader>(_guidMS);
		m_pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);
		m_pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Skinned, _msHandle);

		const auto _guidAS = _assetDB.GetGUIDFromFilePath("Asset/Shader/Source/Geometry/MeshShader/TestAS.cso");
		const auto _asHandle = _resManager.LoadImmediate<Resource::Shader>(_guidAS);
		m_pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Static, _asHandle);
		m_pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Skinned, _asHandle);

		// 深度だけを書くのでピクセルシェーダーは持たない
		m_defaultPSHandle = {};

		m_rootSigHandle = _pPSOManager->Request("Asset/Shader/Source/Geometry/MeshShader/UberMS.cso");

		// 深度テスト有効・書き込み有効
		m_pipelineBuilder.SetDepthConfig(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	}

	void ZPrePass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		if (!_pCtx) return;

		_pCtx->BindCopyHeapAndSumplerBindLess();
		_pCtx->SetGraphicsRootSignature(m_rootSigHandle);

		_pCtx->BindCamera();
		_pCtx->BindMeshInstance();
		_pCtx->BindMeshlet();

		_pCtx->DrawQueueDispathMesh(GetPassIndex());
	}

	EPassEditResult ZPrePass::EditUpdate()
	{
		ImGui::TextDisabled("不透明モデルの深度だけを書きます");
		ImGui::Text("PassIndex : %d", static_cast<int>(GetPassIndex()));
		return EPassEditResult::None;
	}

	void ZPrePass::EditNode()
	{}

	void ZPrePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
