#pragma once

#include "../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// パーティクルアセットの編集・詳細表示
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pParticles">編集対象のパーティクルアセット</param>
	void ParticleEdit(EditorContext& a_editContext, Resource::ParticlesAsset* a_pParticles);
}
