#pragma once

#include "../OffScreenPass.h"

class FullScreenPass final : public OffScreenPass
{
public:

	void Excute(RenderContext* a_pCtx) override;

private:

	void CreatePass() override;
};
