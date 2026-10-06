#pragma once
//==========================================================================================
//
// Editor 共通
//
// エディターは一番上の層(Editor → App → Engine → Core)で、Engine の型をそのまま扱う道具。
// Engine の名前を Editor の中から Engine:: を付けずに使えるよう、Engine を丸ごと取り込む。
//
// Editor の中で GUID を書くときは Core::GUID と書くこと
// (Engine 経由で見える Core::GUID と Windows の ::GUID が区別できなくなるため)。
//
// エディターの外から読まれるヘッダー(Editor.h など)も EditorPCH に頼れないので、
// Editor 配下のヘッダーはすべてこれを読む。
//
//==========================================================================================
#include "Engine/EngineCommon.h"

namespace Editor
{
	using namespace Engine;
}
