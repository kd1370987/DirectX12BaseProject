#pragma once

namespace Engine::Resource
{
	// ハンドル
	template<typename Asset>
	struct Handle
	{
		// 管理場所と世代
		Engine::Resource::Index idx = Engine::Resource::Limits::INVALID_INDEX;
		Engine::Resource::Generation gen = Engine::Resource::Limits::INVALID_GENERATION;
	};
}