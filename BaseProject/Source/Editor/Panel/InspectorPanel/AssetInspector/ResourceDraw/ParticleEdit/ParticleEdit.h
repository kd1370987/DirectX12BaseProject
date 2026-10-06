#pragma once

#include "../../../../../Internal/EditorContext.h"
#include "Editor/EditorCommon.h"

namespace Editor::Inspector
{
	/// <summary>
	/// パーティクルアセットの編集・詳細表示
	/// </summary>
	/// <param name="a_services">アセット一覧と保存先の解決に使う</param>
	/// <param name="a_guid">編集対象のGUID : 保存先はここから引く</param>
	/// <param name="a_pParticles">編集対象のパーティクルアセット</param>
	/// <param name="a_pEditContext">
	/// 参照しているテクスチャを、押すと飛べるリンクにする。nullptr なら名前を出すだけ
	/// </param>
	/// <remarks>
	/// アセットインスペクターとエフェクトエディター(エフェクトが参照している粒を
	/// その場で詰められるように)の両方から呼ばれる
	/// </remarks>
	void ParticleEdit(
		const ECS::EngineServices& a_services,
		const Core::GUID& a_guid,
		Resource::ParticlesAsset* a_pParticles,
		EditorContext* a_pEditContext = nullptr);
}
