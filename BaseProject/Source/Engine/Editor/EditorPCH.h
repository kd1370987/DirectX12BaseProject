#pragma once
//==========================================================================================
//
// EditorPCH
//
// エディター配下(Source/Engine/Editor)だけが使うプリコンパイル済みヘッダー。
// 共通の Pch.h の中身に、エディターだけが使う ImGui 一式と内部の共通部品を足したもの。
// エディター配下の .cpp には、共通の Pch.h の代わりにこれが強制インクルードされる
// (設定は vcxproj。配下に .cpp を足したら、Pch ではなくこちらを使うようにすること。
//  付け忘れはビルド時の CheckEditorPch で止まる)。
//
// ImGui をエディターの外へ漏らさないために、共通の Pch.h からは ImGui を外してある。
// エディターの外から読まれるヘッダー(Editor.h / EffectEditor.h / EditorCamera.h /
// Helper/EditorField.h / Helper/EditorField.inl)は、ここに頼らずに書くこと。
//
//==========================================================================================
#include "Pch.h"

//---------------------------------------------------------
// ImGui
//
// バックエンド(imgui_impl_*)は初期化とフレーム開始を行う ImGuiContext.cpp だけで読む
//---------------------------------------------------------
#pragma warning(push, 0)
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imnodes.h>			// ノードエディタ
#include <imGuizmo.h>			// シーンビューのギズモ
#pragma warning(pop)

//---------------------------------------------------------
// エディター内部の共通部品
//---------------------------------------------------------
#include "Helper/EditorHelper.h"	// 検索欄・SRV表示・ノード部品
