#pragma once

#include "../DescriptorHeap.h"

class CBVHeap : public DescriptorHeap
{
public:
	DescriptorHandle Register(ID3D12Resource* a_resource = nullptr) override;
};