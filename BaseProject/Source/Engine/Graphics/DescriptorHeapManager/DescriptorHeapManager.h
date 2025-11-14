#pragma once

class DescriptorHeap;

class DescriptorHeapManager
{
public:



private:

	DescriptorHeap* m_descriptorHeap;

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