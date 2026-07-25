#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderPassRegistry;

	// 影用スペースデノイズ(à-trous)を登録する。
	// テンポラル蓄積後の影(AfterDLShadowTempAccumu)を入力に、GBufferと同解像度で
	// エッジ保持フィルタをかけて ShadowDenoised へ出力する。
	// これによりテンポラルの履歴依存(=ゴースト要因)を下げてもノイズが残らないようにする。
	void AddShadowSpatialDenoisePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase);
}
