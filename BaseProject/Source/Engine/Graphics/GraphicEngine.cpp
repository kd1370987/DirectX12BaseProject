#include "GraphicEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "../D3D12/PipelineStateManager/PipelineStateManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "RenderContext/ShapeDraw/ShapeDraw.h"
#include "RenderGraph/RenderGraph.h"


namespace Engine::Graphics
{
	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(const GraphicsEngineDesc& a_desc)
	{

		m_pPipelineStateManager = a_desc.pPipelineStateManager;
		// マネージャー構築
		CreateManager();
		
		// ルートシグネチャの定義
		RootSigDefinition();


		// 形状描画クラス構築
		m_upShapeRender = std::make_unique<ShapeRenderer>();

		// レンダーコンテキストの作成
		for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			auto _upCtx = std::make_unique<RenderContext>();

			RenderContextDesc _desc = {};
			_desc.pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

			_desc.pRootSigMana = m_upRootSignatureManager.get();
			_desc.pPSOMana = m_upGrahicsPSOManager.get();
			_desc.pShapeRender = m_upShapeRender.get();

			_desc.cbAllocatorMemSize = 32 * 1024 * 1024;

			_upCtx->Init(_desc);
			m_upRenderContextVec.push_back(std::move(_upCtx));
		}

		// レンダーグラフの構築
		m_upRenderGraph = std::make_unique<RenderGraph>();
		m_upRenderGraph->Init(
			m_upRootSignatureManager.get(),
			m_upGrahicsPSOManager.get(),
			m_pPipelineStateManager
		);
	}

	void GraphicsEngine::Release()
	{}

	void GraphicsEngine::ExcuteDrawCmd()
	{
		m_upRenderContextVec[m_currentFrameIndex]->Excute(m_upRenderGraph.get());
		m_upRenderContextVec[m_currentFrameIndex]->ClearCmd();
	}

	void GraphicsEngine::BegineFrame()
	{
		m_currentFrameIndex = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();
		FrameDesc _desc;
		_desc.pCmdList = D3D12::D3D12Wrapper::Instance().GetCommandList();
		_desc.pCmdListClass = D3D12::D3D12Wrapper::Instance().GetCmdList();
		m_upRenderContextVec[m_currentFrameIndex]->Begine(_desc);
	}
	void GraphicsEngine::EndFrame()
	{
	}

	const Graphics::RenderContext* GraphicsEngine::GetRenderContext() const
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
	}
	Graphics::RenderContext* GraphicsEngine::RefRenderContext()
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
		
	}
	void GraphicsEngine::CreateManager()
	{
		// デバイス取得
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();


		// グラフィックスパイプラインステートマネージャー構築
		m_upGrahicsPSOManager = std::make_unique<D3D12::GraphicsPSOManager>();
		m_upGrahicsPSOManager->Init(_pDevice);

		// ルートシグネチャマネージャー
		m_upRootSignatureManager = std::make_unique<D3D12::RootSignatureManager>();
		m_upRootSignatureManager->Init(100);
	}
	void GraphicsEngine::RootSigDefinition()
	{
		if (!m_upRootSignatureManager) return;

		// ルートシグネチャの定義
		m_upRootSignatureManager->CreateRootSig(
			"BaseRootSig",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
				{RootParameterType::DescriptorTable,
				{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
				RootSigSemantic::MaterialSRV,false}
			},
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
		);
		D3D12::RootSignatureDesc _desc;
		_desc.AddRoot(RootParameterType::RootCBV,0);
		_desc.AddRoot(RootParameterType::RootCBV,1);
		_desc.AddRoot(RootParameterType::RootCBV,2);
		_desc.AddRoot(RootParameterType::RootCBV,3);
		_desc.AddRoot(RootParameterType::RootCBV,4);
		_desc.AddDescriptorHeap(
			{ {RangeType::SRV,0}, { RangeType::SRV,1 }, { RangeType::SRV,2 }, { RangeType::SRV,3 } }
		);
		_desc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		m_pPipelineStateManager->Request(_desc);
		D3D12::RootSignatureDesc _sDesc;
		_sDesc.AddRoot(RootParameterType::RootCBV, 0);
		_sDesc.AddRoot(RootParameterType::RootCBV, 1);
		_sDesc.AddRoot(RootParameterType::RootCBV, 2);
		_sDesc.AddRoot(RootParameterType::RootCBV, 3);
		_sDesc.AddRoot(RootParameterType::RootCBV, 4);
		_sDesc.AddDescriptorHeap(
			{ {RangeType::SRV,0}, { RangeType::SRV,1 }, { RangeType::SRV,2 }, { RangeType::SRV,3 } }
		);
		_sDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		m_pPipelineStateManager->Request(_sDesc);

		m_upRootSignatureManager->CreateRootSig(
			"ForwardLithingPass",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::AmbientCB,true},
				{RootParameterType::DescriptorTable,
				{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
				RootSigSemantic::MaterialSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"QuadRendering",
			{
				{RootParameterType::DescriptorTable,{RangeType::SRV},
				RootSigSemantic::PostScreenSRV,false},
				{RootParameterType::DescriptorTable,{RangeType::SRV},
				RootSigSemantic::PostScreenSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"DeferredLighting",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::AmbientCB,true},
				{RootParameterType::DescriptorTable,
				{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
				RootSigSemantic::PostScreenSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"2DRootSig",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::UICB,true},
				{RootParameterType::DescriptorTable,{RangeType::SRV},RootSigSemantic::MaterialSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"DebugLine",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				//{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
			}
		);

		// テスト用
		m_upRootSignatureManager->CreateRootSig(
			"TestSig",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MaterialIndexCB,true}
			},
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
		);

		// レイ用ルートシグネチャ
		RootSigInit _rayGlobal = {};
		_rayGlobal.isUseStaticSampler = true;
		_rayGlobal.AddRoot(RootParameterType::RootCBV, 0);		// カメラ
		_rayGlobal.AddRoot(RootParameterType::RootSRV, 0);		// TLAS
		_rayGlobal.AddDescriptorHeap({ {RangeType::UAV,0} });	// 出力
		_rayGlobal.AddDescriptorHeap({ {RangeType::SRV,1} });	// インスタンス配列
		_rayGlobal.AddDescriptorHeap({ {RangeType::SRV,2} });	// マテリアル
		_rayGlobal.flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		m_upRootSignatureManager->CreateRootSig(
			"RayGlobal",
			_rayGlobal
		);

		// レイジェネレーション
		RootSigInit _rayGenSigInit = {};
		_rayGenSigInit.isUseStaticSampler = false;
		_rayGenSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		m_upRootSignatureManager->CreateRootSig(
			"RayGen",
			_rayGenSigInit
		);
		// ヒットシェーダー用
		RootSigInit _hitSigInit = {};
		_hitSigInit.isUseStaticSampler = false;
		_hitSigInit.AddDescriptorHeap({ {RangeType::SRV,3},{RangeType::SRV,4},{RangeType::SRV,5},{RangeType::SRV,6} });
		_hitSigInit.AddDescriptorHeap({ {RangeType::SRV,7} });
		_hitSigInit.AddDescriptorHeap({ { RangeType::SRV,8 } });
		_hitSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		m_upRootSignatureManager->CreateRootSig(
			"Hit",
			_hitSigInit
		);
		// missシェーダー用
		RootSigInit _missSigInit = {};
		_missSigInit.isUseStaticSampler = false;
		_missSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		m_upRootSignatureManager->CreateRootSig(
			"Miss",
			_missSigInit
		);
	}
}