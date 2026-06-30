#pragma once
namespace Engine::Graphics
{
	enum class EShaderPermutationFlags : uint32_t
	{
		None = 0,
		Skinned = 1 << 0,		// アニメーションモデルか
		AlphaMasked = 1 << 1,	// アルファテストありか
		UseNormalMap = 1 << 2,	// 法線マップを使っているか
	};

	struct PSOKey
	{
		uint8_t passIndex;                // どのパスか（RenderPassRegistryから取得）
		Engine::GUID shadingModelGUID;    // どのシェーディングモデルか（PBR, Water等）
		uint32_t permutationFlags;        // マテリアルやモデルの状態

		// ハッシュ計算用
		bool operator==(const PSOKey& other) const {
			return passIndex == other.passIndex &&
				shadingModelGUID == other.shadingModelGUID &&
				permutationFlags == other.permutationFlags;
		}
	};
}