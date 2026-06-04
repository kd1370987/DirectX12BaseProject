#pragma once

#include "../BaseRenderPass.h"

namespace Engine::Graphics
{
	// 描画用パスクラス
	class CopyPass : public BaseRenderPass
	{
	public:

		CopyPass() = default;
		virtual ~CopyPass() override = default;

	};
}