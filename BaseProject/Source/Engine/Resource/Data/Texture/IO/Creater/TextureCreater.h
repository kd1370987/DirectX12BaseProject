#pragma once
namespace Engine::Resource
{
	// テクスチャの仕様書を組む
	//
	// 実体の生成そのものは GPUResource::Create が持つので、ここは組むだけにする。
	// レンダーグラフの占有サイズの見積もり(GetResourceAllocationInfo)も、
	// 実体と同じ仕様書を渡さないと確保した席に収まらないので、必ずここを通すこと
	D3D12_RESOURCE_DESC BuildTextureResourceDesc(const TextureCreateDesc& a_desc);

	// 生成時に渡すクリアバリューを組む : RTV でも DSV でもなければ空を返す
	//
	// RTV / DSV は生成時に渡しておかないと、クリアのたびにドライバ側で
	// 最適化が効かず警告も出る
	std::optional<D3D12_CLEAR_VALUE> BuildTextureClearValue(
		const TextureCreateDesc& a_desc,
		const D3D12_RESOURCE_DESC& a_resourceDesc
	);
}
