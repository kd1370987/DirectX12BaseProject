#include "TestFuncPass.h"

#include "../RenderGraph/RGData/RenderPassNode.h"
#include "../RenderGraph/RenderGraph.h"
#include "../GraphicEngine.h"
#include "../RenderGraph/RGPassBuilder/RGPassBuilder.h"

namespace Engine::Graphics
{
	void AddTestFuncPass(D3D12::PipelineStateManager* a_pPSOManager,RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// 実行時用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t testSIndex;
			uint8_t testDIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		RGRasterPassBuilder _rpBuilder(&_node, &a_rg);

		// パス共通設定作成
		_rpBuilder.SetRootSignature(a_pPSOManager,"Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		// 読み込み
		_rpBuilder.Read("Depth", AccessType::Depth_Read, LoadOp::Load, StoreOp::Store);

		// 書き出し
		_rpBuilder.Write("GBufferAlbedo", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferNormal", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferMaterial", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferEmissiv", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferVelocity", AccessType::RTV, LoadOp::Clear, StoreOp::Store);

		// ---- PSO設定作成 ----
		// アニメーションナシ
		auto& _sPso = _rpBuilder.CreatePSODesc("TestS",_spPassData->testSIndex);
		_rpBuilder.SetVS(
			_sPso,
			"Asset/Shader/Source/GBufferShader/GBufferVS.cso",
			D3D12::Input::StaticLayout
		);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
		// あり
		auto& _aPso = _rpBuilder.CreatePSODesc("TestS",_spPassData->testDIndex);
		_rpBuilder.SetVS(
			_aPso,
			"Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso",
			D3D12::Input::AnimationInputLayout
		);
		_rpBuilder.SetPS(_aPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_aPso.DepthEnable(true);
		_aPso.DepthWriteMask(false);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// 全PSOを作成
		_rpBuilder.ResolveAndCompile();


		// パスとして登録
		a_rg.AddPassNode(a_phase,_node);
	}
}