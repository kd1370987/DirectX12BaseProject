#pragma once

#include "../OffScreenPass.h"

class MonotonePass final : public OffScreenPass
{
public:

	void Excute(RenderContext* a_pCtx) override;
};
