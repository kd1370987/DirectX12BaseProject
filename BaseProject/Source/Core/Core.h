#pragma once

// Coreは基本的にOS,DirectXなどの依存がない物をまとめるディレクトリ

// ファイルユーティリティ
#include "Utility/FileUtility.h"
// 文字列ユーティリティ
#include "Utility/StringUtillity.h"

// アライメント
#include "Math/Alignment.h"

// 汎用ストレージクラス
//#include "Storage/Storage.h";					 
// 連続性が保証されているスロット
#include "Storage/FreeRange/FreeRange.h"

// アルゴリズム

// グラフ
#include "Algorithm/Graph/TopologicalSort.h"
#include "Algorithm/Graph/GroupTopologicalSort.h"