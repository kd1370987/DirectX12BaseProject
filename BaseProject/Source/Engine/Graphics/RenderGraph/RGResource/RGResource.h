#pragma once

namespace Engine::Graphic
{
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

	// レンダーグラフで使うリソースクラス
	// レンダーターゲットや深度値などのテクスチャを扱う
	class RGResource
	{
	public:


	private:

		// リソース設定
		Resource::ID m_id = Resource::Limits::INVALID_ID;	// 識別ID
		std::string m_name = "none";						// リソースネーム
		ResourceUsage m_usage = ResourceUsage::None;		// 使用フラグ

		// 実行中のステータス
		D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;

		
		
	};
}