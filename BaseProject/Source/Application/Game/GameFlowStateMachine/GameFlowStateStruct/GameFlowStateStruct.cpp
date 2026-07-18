#include "GameFlowStateStruct.h"

namespace App::Game
{
	void FlowNode::Archive(Engine::Persistence::Archive& a_arch)
	{
		// 共通の「つなぎ情報」(名前・座標・各種ID)
		ArchiveTopology(a_arch);

		// GameFlow 固有: 読み込むシーンのGUID
		a_arch.Field("sceneGUID", sceneGUID);
	}
}
