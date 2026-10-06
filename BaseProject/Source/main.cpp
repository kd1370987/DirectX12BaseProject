#include "Application/App.h"
#include "Engine/MainEngine.h"
#include "Editor/Editor.h"

//==========================================================================================
// エントリポイント(組み立ての場所)
//
// 依存の向きは Editor → App → Engine → Core。どの層も上の層を知らないので、
// 全部を知っているこのファイルだけが部品をつなぐ。
// エディターはここで開発ツールの窓口(Engine::DevTool::IDevTool)としてエンジンへ差し込む。
// 差し込まなければ、エンジンとゲームはエディター無しで動く
//==========================================================================================

/// <summary>
/// アプリケーションのエントリポイント
/// </summary>
/// <param name="a_hInstance">アプリケーションインスタンス</param>
/// <param name="a_hPrevInstance">非推奨 : 常にNULL </param>
/// <param name="a_lpCmdLine">コマンドライン引数</param>
/// <param name="a_nCmdShow">ウィンドウ表示状態</param>
/// <returns>終了コード : 通常は 0 </returns>
int WINAPI WinMain(HINSTANCE a_hInstance, HINSTANCE a_hPrevInstance, LPSTR a_lpCmdLine, int a_nCmdShow)
{
	// 開発ツール(エディター)の差し込み : MainEngine::Init より前に済ませる
	Engine::MainEngine::Instance().SetDevTool(&Editor::MainEditor::Instance());

	App::Application _app = {};
	_app.Execute();
	return 0;
}