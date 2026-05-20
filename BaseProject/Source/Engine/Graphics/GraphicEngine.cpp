#include "GraphicEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../D3D12/PipelineStateManager/PipelineStateManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "RenderContext/ShapeDraw/ShapeDraw.h"
#include "RenderGraph/RenderGraph.h"
#include "../Animation/AnimationMatrixManager/AnimationMatrixManager.h"
#include "../Resource/Manager/ResourceManager/ResourceManager.h"


namespace Engine::Graphics
{
	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(const GraphicsEngineDesc& a_desc)
	{

		m_pPipelineStateManager = a_desc.pPipelineStateManager;

		// 形状描画クラス構築
		m_upShapeRender = std::make_unique<ShapeRenderer>();

		// レンダーコンテキストの作成
		for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			auto _upCtx = std::make_unique<RenderContext>();

			RenderContextDesc _desc = {};
			_desc.pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
			_desc.pShapeRender = m_upShapeRender.get();

			_desc.cbAllocatorMemSize = 32 * 1024 * 1024;
			_desc.boneElementNum = 10000;

			_upCtx->Init(_desc);
			m_upRenderContextVec.push_back(std::move(_upCtx));
		}

		// レンダーグラフの構築
		m_upRenderGraph = std::make_unique<RenderGraph>();
		m_upRenderGraph->Init(m_pPipelineStateManager);

		// アニメーション用メモリ領域作成
		auto* _pDev = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _CmdList = D3D12::D3D12Wrapper::Instance().GetCmdList();
		Animation::AnimationMatrixManager::Instance().Init(_pDev, *_CmdList, 10000);
	}

	void GraphicsEngine::Release()
	{}

	void GraphicsEngine::ExcuteDrawCmd()
	{
		// レンダーパスの実行
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
	RenderGraph* GraphicsEngine::RefRenderGraph()
	{
		return m_upRenderGraph.get();
	}
}