#pragma once

namespace Engine::Resource
{
	// ボーン行列ラップ
	struct BoneMatrix {
		DirectX::XMFLOAT4X4 mat;
	};
	// ノードポーズ行列ラップ
	struct NodePoseMatrix {
		DirectX::XMFLOAT4X4 local;
		DirectX::XMFLOAT4X4 world;
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
	enum class TextureUsage : uint32_t
	{
		None = 0,
		RTV = 1 << 0,
		DSV = 1 << 1,
		SRV = 1 << 2,
		UAV = 1 << 3,
	};

	inline TextureUsage operator|(TextureUsage a, TextureUsage b)
	{
		return static_cast<TextureUsage>(
			static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
			);
	}

	inline TextureUsage operator&(TextureUsage a, TextureUsage b)
	{
		return static_cast<TextureUsage>(
			static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
			);
	}

	inline TextureUsage& operator|=(TextureUsage& a, TextureUsage b)
	{
		a = a | b;
		return a;
	}

	inline bool HasFlag(TextureUsage value, TextureUsage flag)
	{
		return (static_cast<uint32_t>(value) &
			static_cast<uint32_t>(flag)) != 0;
	}

	inline D3D12_RESOURCE_FLAGS GetResourceFlags(TextureUsage a_value)
	{
		D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE;

		if (HasFlag(a_value, TextureUsage::RTV))
		{
			_flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (HasFlag(a_value, TextureUsage::DSV))
		{
			_flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (HasFlag(a_value, TextureUsage::UAV))
		{
			_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!HasFlag(a_value, TextureUsage::SRV))
		{
			_flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		return _flags;
	}
}