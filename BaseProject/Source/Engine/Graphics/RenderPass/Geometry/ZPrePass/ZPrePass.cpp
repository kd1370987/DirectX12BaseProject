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
		RGRasterPassBuilder _rpBuilder(&_node);

		// 依存関係構築
		_rpBuilder.WriteDepth("Depth", DXGI_FORMAT_R32_TYPELESS, LoadOp::Clear, StoreOp::Store);

		// =========================================================
		// VS設定 と 深度設定
		// =========================================================
		std::string _staticVSPath = "Asset/Shader/Source/ZPreShader/ZPreVS.cso";
		auto _staticVSHandle = Resource::ShaderLoader::Request(_staticVSPath);
		auto _animationVSHandle = Resource::ShaderLoader::Request("Asset/Shader/Source/ZPreShader/AnimationZPreVS.cso");

		_node.pipelineBuilder.RegisterVertexShader(EShaderPermutationFlags::Static, _staticVSHandle);
		_node.pipelineBuilder.RegisterVertexShader(EShaderPermutationFlags::Skinned, _animationVSHandle);

		// 深度テスト(LESS_EQUAL)と深度書き込み(true)をBuilderに教える
		_node.pipelineBuilder.SetDepthConfig(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// ルートシグネチャの確定
		_spPassData->pRootSig = a_pPSOManager->Request(_staticVSPath);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				CameraData _cbCam = a_pGE->GetCameraData();
				a_pCtx->GraphicsBindRootCBV(0, _cbCam);
				a_pCtx->BindInstanceBuffer(2);
				a_pCtx->BindSubsetBuffer(3);
				a_pCtx->BindBonePalletBuffer(4);

				// 描画
				a_pGE->DrawQueue(a_pCtx, a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
	void AddMeshZPrePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード
		RenderPassNode _node = {};
		_node.name = "MeshZPre";
		_node.phase = a_phase;
		RGMeshShaderPassBuilder _msBuilder(&_node);


		// 依存関係構築
		_msBuilder.WriteDepth("Depth", DXGI_FORMAT_R32_TYPELESS, LoadOp::Clear, StoreOp::Store);

		// シェーダー関係セット
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/UberMS.cso");
		auto _msHandle = Resource::ResourceManager::Instance().Load<Resource::Shader>(_guid);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Skinned, _msHandle);

		// ルートシグネチャセット
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/Mesh/UberMS.cso");

		// 深度テスト(LESS_EQUAL)と深度書き込み(true)をBuilderに教える
		_node.pipelineBuilder.SetDepthConfig(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{

				a_pCtx->BindCopyHeapAndSumplerBindLess();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				a_pCtx->BindCamera();
				a_pCtx->BindMeshInstance();
				a_pCtx->BindMeshlet();

				a_pCtx->DrawQueueDispathMesh(a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}


}
