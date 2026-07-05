#pragma once
namespace Engine::Graphics
{
	
	enum class EShaderPermutationFlags : uint32_t
	{
		None				= 0,
		Static				= 1 << 0,	// スタティックモデルか
		Skinned				= 1 << 1,	// アニメーションモデルか
		AlphaMasked			= 1 << 2,	// アルファテストありか
		UseNormalMap		= 1 << 3,	// 法線マップを使っているか
		DepthMasked			= 1 << 4,	// 深度テストありか
		UseGPUInstancing	= 1 << 5,	// GPUインスタンシング使用するか

	};

	struct PSOKey
	{
		Handle<Resource::ShadingModelTable> shadingModelTableHandle = {};	// どのシェーディングモデルか（PBR, Water等）
		uint32_t permutationFlags;											// マテリアルやモデル、エンティティの状態

		// ハッシュ計算用
		bool operator==(const PSOKey& other) const {
			return	shadingModelTableHandle == other.shadingModelTableHandle &&
					permutationFlags == other.permutationFlags;
		}
	};
}

namespace std
{
	template <>
	struct hash<Engine::Graphics::PSOKey>
	{
		size_t operator()(const Engine::Graphics::PSOKey& key) const
		{
			uint64_t _hash = 14695981039346656037ull;
			auto UpdateHash = [&](const void* p, size_t size) {
				const uint8_t* ptr = static_cast<const uint8_t*>(p);
				for (size_t i = 0; i < size; ++i) {
					_hash ^= ptr[i];
					_hash *= 1099511628211ull;
				}
				};

			UpdateHash(&key.shadingModelTableHandle.id, sizeof(key.shadingModelTableHandle.id));
			UpdateHash(&key.permutationFlags, sizeof(key.permutationFlags));

			return static_cast<size_t>(_hash);
		}
	};
}