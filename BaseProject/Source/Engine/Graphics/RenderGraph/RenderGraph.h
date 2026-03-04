#pragma once

#include "../RenderPass/RenderPass.h"

#include "RGResource/RGTexture/RGTexture.h"

enum class ResourceUsage : uint32_t
{
	None = 0,
	RenderTarget = 1 << 0,
	DepthStencil = 1 << 1,
	ShaderRead = 1 << 2,
	ShaderWrite = 1 << 3,   // UAV 用
};

inline ResourceUsage operator|(ResourceUsage a, ResourceUsage b)
{
	return static_cast<ResourceUsage>(
		static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
		);
}

inline ResourceUsage operator&(ResourceUsage a, ResourceUsage b)
{
	return static_cast<ResourceUsage>(
		static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
		);
}

inline ResourceUsage& operator|=(ResourceUsage& a, ResourceUsage b)
{
	a = a | b;
	return a;
}

inline bool HasFlag(ResourceUsage value, ResourceUsage flag)
{
	return (static_cast<uint32_t>(value) &
		static_cast<uint32_t>(flag)) != 0;
}

struct ResourceDesc
{
	std::string name = "none";
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	uint32_t widht = 1280;
	uint32_t height = 720;

	ResourceUsage usage = ResourceUsage::None;
};


struct RGresourceVersion
{
	uint32_t writerPass;
};

struct RGResource
{
	Resource::ID id = Resource::Limits::INVALID_ID;
	ResourceDesc desc = {};

	// コンパイル後に決まる
	std::shared_ptr<RGTexture> spRGTexture = nullptr;

	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
	uint32_t lastWritePass = 0;

	std::vector<RGresourceVersion> versionVec = {};
};

struct RGBarrier
{
	RGTexture* texture = nullptr;
	D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_COMMON;
	Resource::ID resID = Resource::Limits::INVALID_ID;
};

struct CompiledPass
{
	RenderPass* pPass = nullptr;
	std::vector<RGBarrier> barrierVec = {};
};

class RenderGraph
{
public:

	void Init(
		ShaderManager* a_pShaderMana,
		RootSignatureManager* a_pRootSigMana,
		GraphicsPSOManager* a_pPSOMana
	);							// 初回

	void Release();

	void Compile();							// Pass追加後
	void Excute(RenderContext* a_pCtx);		// パスを順次実行

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const std::string& a_name);

	D3D12_CPU_DESCRIPTOR_HANDLE RTVCPU(const std::string& a_name);

	// リソース作成
	Resource::ID CreateResource(const ResourceDesc& a_desc);
	Resource::ID GetID(const std::string& a_key);

	// パス登録
	template<typename Pass>
	void RegisterPass()
	{
		std::shared_ptr<RenderPass> _pass = std::make_shared<Pass>();
		m_spPassVec.push_back(_pass);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle(const std::string& a_name);

	std::vector<std::string> GetRGResourceList();

private:

	void ReadVersion(uint32_t a_writePass);

	void RiteVersion(uint32_t a_writePass);

private:

	// パスの保管場所
	std::vector<std::shared_ptr<RenderPass>> m_spPassVec = {};
	//std::vector<RenderPass*> m_sortedPassed = {};							// ソート後のパス
	std::vector<std::vector<RenderPass*>> m_groupSortedPassed = {};			// ソート後のパス

	// リソース仕様書のストレージ
	SlotStorage<ResourceDesc> m_resourceStorage = {};
	std::unordered_map<Resource::ID, RGResource> m_rgResourceMap = {};

	// コンパイル後のパス
	std::vector<CompiledPass> m_compiledPasses = {};
};