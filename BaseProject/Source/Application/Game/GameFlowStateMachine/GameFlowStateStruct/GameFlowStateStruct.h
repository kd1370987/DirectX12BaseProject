#pragma once
//==========================================================================================
//
// GameFlow 固有のノード
//
// パラメータ・遷移条件・遷移矢印などの共通部分は Engine::StateGraph 側に移動した。
// ここには「GameFlow のノードが固有に持つデータ(遷移先シーンのGUID)」だけを定義する。
//
//==========================================================================================
#include "Engine/Resource/StateGraph/StateGraphTypes.h"

namespace App::Game
{
	// GameFlow のステートノード
	// StateGraph の共通「つなぎ情報」を継承し、固有データとしてシーンGUIDを持つ。
	struct FlowNode : Engine::StateGraph::StateNodeBase
	{
		Engine::GUID sceneGUID = {};	// このステートで読み込むシーン

		void Archive(Engine::Persistence::Archive& a_arch);
	};
}
