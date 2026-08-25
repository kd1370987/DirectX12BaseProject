#pragma once

#include "../../../../../Internal/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// パーティクルアセットの編集・詳細表示
	/// </summary>
	/// <param name="a_pParticles">編集対象のパーティクルアセット</param>
	/// <param name="a_pEditContext">
	/// 参照しているテクスチャを、押すと飛べるリンクにする。nullptr なら名前を出すだけ
	/// </param>
	/// <remarks>
	/// アセットインスペクターとエフェクトエディター(エフェクトが参照している粒を
	/// その場で詰められるように)の両方から呼ばれる
	/// </remarks>
	void ParticleEdit(
		Resource::ParticlesAsset* a_pParticles,
		EditorContext* a_pEditContext = nullptr);
}
