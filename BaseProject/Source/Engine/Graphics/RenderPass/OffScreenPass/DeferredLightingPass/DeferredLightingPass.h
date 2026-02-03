#pragma once

#include "../OffScreenPass.h"

class DeferredLightingPass final : public OffScreenPass
{
public:

	void Excute(RenderContext* a_pCtx) override;

private:

	void CreatePass() override;
};
