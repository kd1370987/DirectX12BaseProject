#pragma once

#include "../DrawPass.h"

class AnimationGBufferPass final : public DrawPass
{
public:

	void Excute(RenderContext* a_pCtx) override;

private:

	void CreatePass() override;
};
