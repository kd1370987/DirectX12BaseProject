#pragma once

#include "../DescriptorHeap.h"

class SRVHeap : public DescriptorHeap
{
public:

	DescriptorHandle Register(ID3D12Resource* a_resource = nullptr) override;

private:

};