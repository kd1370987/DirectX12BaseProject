#include "Application/App.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	Application::Instance().Execute();
	return 0;
}