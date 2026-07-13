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
		RGMeshShaderPassBuilder _msBuilder(&_node);

		// 依存関係構築
		_msBuilder.ReadDepth("Depth");

		_msBuilder.WriteRTV("GBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_msBuilder.WriteRTV("GBufferNormal", DXGI_FORMAT_R16G16_FLOAT);
		_msBuilder.WriteRTV("GBufferMaterial", DXGI_FORMAT_R8G8B8A8_UNORM);
		_msBuilder.WriteRTV("GBufferEmissiv", DXGI_FORMAT_R8G8B8A8_UNORM);
		_msBuilder.WriteRTV("GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT);

		// シェーダー関係セット
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath("Asset/Shader/Source/Mesh/UberMS.cso");
		auto _msHandle = Resource::ResourceManager::Instance().Load<Resource::Shader>(_guid);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);
		_node.pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Skinned, _msHandle);

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

				a_pCtx->DrawQueueDispathMesh(a_passIndex);
			};

		// パス登録
		a_pRegistry->RegisterPass(_node);
	}
}