#include "GBufferPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"

#include "Engine/Resource/Loader/Shader/ShaderLoader.h"

#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddGBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// オプション取得
		const auto& _renderingOption = Option::OptionManager::GetInstance().GetRenderingOption();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "GBuffer";
		_node.phase = a_phase;
		RGRasterPassBuilder _rpBuilder(&_node);

		// 依存関係構築
		_rpBuilder.ReadDepth("Depth");

		_rpBuilder.WriteRTV("GBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_rpBuilder.WriteRTV("GBufferNormal", DXGI_FORMAT_R16G16_FLOAT);
		_rpBuilder.WriteRTV("GBufferMaterial", DXGI_FORMAT_R8G8B8A8_UNORM);
		_rpBuilder.WriteRTV("GBufferEmissiv", DXGI_FORMAT_R8G8B8A8_UNORM);
		_rpBuilder.WriteRTV("GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT);

		// VSセット
		auto _staticVSHandle = Resource::ShaderLoader::Request("Asset/Shader/Source/GBufferShader/GBufferVS.cso");
		auto _animationVSHandle = Resource::ShaderLoader::Request("Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso");
		_node.pipelineBuilder.RegisterVertexShader(EShaderPermutationFlags::Static, _staticVSHandle);
		_node.pipelineBuilder.RegisterVertexShader(EShaderPermutationFlags::Skinned, _animationVSHandle);

		// ルートシグネチャセット : VSからもらう
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		// 深度値テスト
		if (_renderingOption.isZPre)
		{
			// Zpreが有効ならば、GBufferは深度テストのみ行い深度書き込みはしない
			_node.pipelineBuilder.SetDepthConfig(
				true,							// 深度テスト有効
				false,							// 書き込み無効
				D3D12_COMPARISON_FUNC_EQUAL		// ZPreと完全に一致するピクセルだけ描画
			);
		}
		else
		{
			_node.pipelineBuilder.SetDepthConfig(
				true,
				true,
				D3D12_COMPARISON_FUNC_LESS_EQUAL
			);
		}

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				a_pCtx->BindHeap();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

				CameraData _cbCam = a_pGE->GetCameraData();
				auto* _pCmd = a_pCtx->GetCurrentCmdList();
				a_pCtx->BindCB()->BindAndAttachDataRootCBV<CameraData>(_pCmd, 0, _cbCam);
				a_pCtx->BindInstanceBuffer(2);
				a_pCtx->BindSubsetBuffer(3);
				a_pCtx->BindBonePalletBuffer(4);

				// 描画
				a_pGE->DrawQueue(a_pCtx, a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
	void AddMeshGBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード
		RenderPassNode _node = {};
		_node.name = "MeshGBuffer";
		_node.phase = a_phase;
		RGMeshShaderPassBuilder _msBuilder(&_node);

		// 依存関係構築
		_msBuilder.ReadDepth("Depth");

		_msBuilder.WriteRTV("MeshGBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_msBuilder.WriteRTV("MeshGBufferNormal", DXGI_FORMAT_R16G16_FLOAT);
		_msBuilder.WriteRTV("MeshGBufferMaterial", DXGI_FORMAT_R8G8B8A8_UNORM);
		_msBuilder.WriteRTV("MeshGBufferEmissiv", DXGI_FORMAT_R8G8B8A8_UNORM);
		_msBuilder.WriteRTV("MeshGBufferVelocity", DXGI_FORMAT_R16G16_FLOAT);

		// シェーダー関係セット
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/UberMS.cso");
		auto _msHandle = Resource::ResourceManager::Instance().Load<Resource::Shader>(_guid);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);

		// ルートシグネチャセット
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/Mesh/UberMS.cso");

		// 深度テスト設定
		_node.pipelineBuilder.SetDepthConfig(
			true,							// 深度テスト有効
			false,							// 書き込み無効
			D3D12_COMPARISON_FUNC_EQUAL		// ZPreと完全に一致するピクセルだけ描画
		);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				a_pCtx->BindCopyHeapAndSumplerBindLess();
				a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);
				

				a_pCtx->BindCamera();
				a_pCtx->BindMeshInstance();
				a_pCtx->BindMeshlet();
				//a_pCtx->BindBonePalletBuffer(7);

				a_pCtx->DrawQueueDispathMesh(a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}