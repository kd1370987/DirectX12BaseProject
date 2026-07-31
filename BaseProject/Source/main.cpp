#include "Application/App.h"

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
	Application _app = {};
	_app.Execute();
	return 0;
}