#pragma once
namespace Engine::Animation
{
	struct SkiningMeshData
	{
		// コンピュートシェーダーで書き込む変形後のバッファ
		RangeHandle<Resource::MeshVertexFloat> animatedVertexHandle = {};

		// どのメッシュの参照先か
		Handle<Resource::Mesh> meshHandle;
	};

	struct AnimatedMeshVertex
	{
		std::vector<Animation::SkiningMeshData> meshDataVec = {};
	};
}