#include "GPUProfiler.h"

#include "../../Internal/EditorTimer.h"

namespace Engine::Editor
{
	void GPUProfiler::Init()
	{

	}
	void GPUProfiler::BeginFrame()
	{
		m_nextQuery = 0;
	}

	void GPUProfiler::EndFrame()
	{
		
	}

	void GPUProfiler::Start(D3D12::GraphicsCommandList* a_pCmdList, Timer& a_timer)
	{}

	void GPUProfiler::End(D3D12::GraphicsCommandList * a_pCmdList, Timer & a_timer)
	{}

}





