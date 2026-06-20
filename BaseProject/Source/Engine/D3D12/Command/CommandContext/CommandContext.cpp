#include "CommandContext.h"

#include "../CommandPool/CommandPool.h"

namespace Engine::D3D12
{
	Engine::D3D12::CommandContext::CommandContext()
	{}
	CommandContext::~CommandContext()
	{}
	void CommandContext::Init(Device * a_pDevice)
	{
		m_upDirectCmdPool = std::make_unique<CommandPool>();
		m_upCopyCmdPool = std::make_unique<CommandPool>();
		m_upComputeCmdPool = std::make_unique<CommandPool>();

		// 初期化
		m_upDirectCmdPool->Init(a_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_upCopyCmdPool->Init(a_pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
		m_upComputeCmdPool->Init(a_pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	}
	CommandPool* CommandContext::RefDirectPool()
	{
		return m_upDirectCmdPool.get();
	}
	CommandPool* CommandContext::RefCopyPool()
	{
		return m_upCopyCmdPool.get();
	}
	CommandPool* CommandContext::RefComputePool()
	{
		return m_upComputeCmdPool.get();
	}
}