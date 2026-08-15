#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderPassRegistry;

	// 汎用のガウシアンブラーパス。
	//
	// 入力と出力で解像度が違ってよいので、1つで縮小(ダウンサンプリング)にも
	// 拡大(アップサンプリング)にも使える。ブルームの縮小バッファ作りが主な用途。
	//
	// 縮小と拡大でタップ数の重さが全く違う(拡大側はフル解像度で回る)ので、
	// σとタップ半径は呼び出し側から個別に渡す形にしている。
	//
	// a_srcScale : 入力テクスチャの解像度スケール(ウィンドウ解像度に対する倍率)
	//              シェーダーへ渡す1テクセルぶんのUVを求めるために必要
	// a_dstScale : 出力テクスチャの解像度スケール。この解像度でテクスチャが確保される
	// a_sigma    : ガウス分布の標準偏差(入力テクセル単位)
	// a_tapRadius: 片側のタップ数。総タップ数は (a_tapRadius * 2 + 1)^2 になる
	void AddGaussianBlurPass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase,
		const std::string& a_passName,
		const std::string& a_srcName,
		const std::string& a_dstName,
		float a_srcScale,
		float a_dstScale,
		float a_sigma,
		int a_tapRadius,
		DXGI_FORMAT a_format = DXGI_FORMAT_R16G16B16A16_FLOAT
	);
}
