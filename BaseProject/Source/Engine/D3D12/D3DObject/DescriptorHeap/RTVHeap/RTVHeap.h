#pragma once

#include "../DescriptorHeap.h"

class RTVHeap : public DescriptorHeap
{
public:

	DescriptorHandle Register(ID3D12Resource* a_resource = nullptr) override;
};