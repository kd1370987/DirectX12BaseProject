#pragma once

#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// UIPass
	//
	// UIを画面へ重ねる。深度を持たないので、積んだ順がそのまま前後になる。
	// 並べ替えは GraphicsEngine 側(レイヤー順)で済んでいる
	//======================================================================================
	class UIPass : public Pass
	{
	public:
		~UIPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;
	};
}
