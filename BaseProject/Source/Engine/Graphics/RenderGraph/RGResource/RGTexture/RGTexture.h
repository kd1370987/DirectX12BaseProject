#pragma once

struct RGTextureDesc
{
	uint32_t width = 0;
	uint32_t height = 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	uint32_t mipLevel = 1;
	uint32_t sampleCount = 1;

	bool allowRTV = false;
	bool allowDSV = false;
	bool allowSRV = true;
	bool allowUAV = false;

	std::optional<D3D12_CLEAR_VALUE> clearValue;
};

class RGTexture
{
public:

	bool Create(const RGTextureDesc& a_desc);

	ID3D12Resource* GetResource() const { return m_cpResource.Get(); }

	const RGTextureDesc& GetDesc()const { return m_desc; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();

private:

	ComPtr<ID3D12Resource> m_cpResource = nullptr;

	RGTextureDesc m_desc;

	RTVHandle		 m_rtvHandle{};
	Storage::Range	 m_srvHandle;
};