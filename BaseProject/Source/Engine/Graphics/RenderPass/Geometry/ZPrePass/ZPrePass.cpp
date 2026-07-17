#include "ZPrePass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../RenderPassRegistry/RenderPassRegistry.h"
#include "../../../../Resource/Loader/Shader/ShaderLoader.h"

#include "../../../../Option/OptionManager.h"
namespace Engine::Graphics
{

	void AddZPrePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ★ ここで OptionManager を見て、オフならそもそもパスを登録せずにリターンする！
		// if (!Engine::System::OptionManager::Instance().IsZPreEnabled()) return;

		struct RuntimeData {
			ID3D12RootSignature* pRootSig = nullptr;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		RenderPassNode _node = {};
		_node.name = "ZPre";
		_node.phase = a_phase;
		RGMeshShaderPassBuilder _msBuilder(&_node);


		// 依存関係構築
		_msBuilder.WriteDepth("Depth", DXGI_FORMAT_R32_TYPELESS, LoadOp::Clear, StoreOp::Store);

		// シェーダー関係セット
		auto _guidMS = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/UberMS.cso");
		auto _msHandle = Resource::ResourceManager::Instance().Load<Resource::Shader>(_guidMS);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Skinned, _msHandle);

		auto _guidAS = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/TestAS.cso");
		auto _asHandle = Resource::ResourceManager::Instance().Load<Resource::Shader>(_guidAS);
		_node.pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Static, _asHandle);
		_node.pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Skinned, _asHandle);

		// ルートシグネチャセット
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/Mesh/UberMS.cso");

		// 深度テスト(LESS_EQUAL)と深度書き込み(true)をBuilderに教える
		_node.pipelineBuilder.SetDepthConfig(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{

				a_pCtx->BindCopyHeapAndSumplerBindLess();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				a_pCtx->BindCamera();
				a_pCtx->BindMeshInstance();
				a_pCtx->BindMeshlet();

				a_pCtx->DrawQueueDispathMesh(a_res.PassIndex());
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}
