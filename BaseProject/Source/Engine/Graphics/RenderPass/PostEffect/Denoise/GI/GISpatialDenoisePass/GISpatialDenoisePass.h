#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderPassRegistry;

	// GI用スペースデノイズ(à-trous)を登録する。
	// 入出力リソース名・パス数・フォーマットを差し替えられるようにして、
	// テンポラルデノイズ「後」の本処理と「前」のプリデノイズの両方で使い回せるようにしている。
	//   a_passName  : 登録するパス名の接頭辞(中間バッファ名もここから生成するので一意にすること)
	//   a_inputRes  : 入力リソース名
	//   a_outputRes : 最終出力リソース名
	//   a_passCount : à-trous の反復回数(1ならプリデノイズのように1回だけかける)
	//   a_format    : 中間/出力バッファのフォーマット
	void AddGISpatialDenoisePass(
		D3D12::PipelineStateManager* a_pPSOManager,
		RenderPassRegistry* a_pRegistry,
		const EDrawPhase& a_phase,
		const std::string& a_passName = "GISpatialDenoisePass",
		const std::string& a_inputRes = "DenoiseGI",
		const std::string& a_outputRes = "FinalGI",
		int a_passCount = 5,
		DXGI_FORMAT a_format = DXGI_FORMAT_R8G8B8A8_UNORM);
}