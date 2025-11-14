#pragma once

class DescriptorHeap;
struct DescriptorHandle;

class DescriptorHeapManager
{
public:

	void Init();

	DescriptorHandle RegisterSRV(ID3D12Resource* a_resource);

private:

	std::shared_ptr<DescriptorHeap> m_spSRVHeap = nullptr;

// シングルトン
private:
	DescriptorHeapManager() = default;
	~DescriptorHeapManager() = default;

	// コピー禁止
	DescriptorHeapManager(const DescriptorHeapManager&) = delete;
	void operator=(const DescriptorHeapManager&) = delete;

public:

	static DescriptorHeapManager& Instance()
	{
		static DescriptorHeapManager instance;
		return instance;
	}
};