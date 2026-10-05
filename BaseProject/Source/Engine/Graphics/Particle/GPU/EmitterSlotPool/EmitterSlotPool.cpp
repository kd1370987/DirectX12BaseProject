#include "EmitterSlotPool.h"
namespace Engine::Particle
{
	void Engine::Particle::EmitterSlotPool::Expand()
	{
		// 新規にブロックを作成
		auto& _upBlock = m_upEmitterBlocks.emplace_back(std::make_unique<EmitterBlock>(m_blockSize));
	}
	EmitterSlotPool::EmitterBlock::EmitterBlock(size_t a_num)
	{
		emitterSlotsBuffer.Create();
		handlePool.Create(a_num);
	}
	EmitterSlotPool::EmitterBlock::~EmitterBlock()
	{
	}
}