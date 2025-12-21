#include "Application/App.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	Application::Instance().Excute();
	return 0;
}