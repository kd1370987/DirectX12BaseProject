#pragma once

namespace Engine::Resource
{
	// ボーン行列ラップ
	struct BoneMatrix {
		Math::Matrix mat;
	};
	// ノードポーズ行列ラップ
	struct NodePoseMatrix {
		Math::Matrix local;
		Math::Matrix world;
	};

	// スロット管理
	template<typename Data>
	struct SharedSlot
	{
		Data data;
		Generation gen = Limits::INVALID_GENERATION;
		uint32_t sharedCount = 0;
	};

	// テクスチャの使用方法
	enum class ETextureUsage : uint32_t
	{
		None = 0,
		RTV = 1 << 0,
		DSV = 1 << 1,
		SRV = 1 << 2,
		UAV = 1 << 3,
	};

	inline ETextureUsage operator|(ETextureUsage a, ETextureUsage b)
	{
		return static_cast<ETextureUsage>(
			static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
			);
	}

	inline ETextureUsage operator&(ETextureUsage a, ETextureUsage b)
	{
		return static_cast<ETextureUsage>(
			static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
			);
	}

	inline ETextureUsage& operator|=(ETextureUsage& a, ETextureUsage b)
	{
		a = a | b;
		return a;
	}

	inline bool HasFlag(ETextureUsage value, ETextureUsage flag)
	{
		return (static_cast<uint32_t>(value) &
			static_cast<uint32_t>(flag)) != 0;
	}

	inline D3D12_RESOURCE_FLAGS GetResourceFlags(ETextureUsage a_value)
	{
		D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE;

		if (HasFlag(a_value, ETextureUsage::RTV))
		{
			_flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (HasFlag(a_value, ETextureUsage::DSV))
		{
			_flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (HasFlag(a_value, ETextureUsage::UAV))
		{
			_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!HasFlag(a_value, ETextureUsage::SRV) && HasFlag(a_value,ETextureUsage::DSV))
		{
			_flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		return _flags;
	}
}