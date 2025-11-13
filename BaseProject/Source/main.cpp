#include "Application/App.h"

int wmain(int argc, wchar_t** argv, wchar_t** envp)
{
	Application::Instance().Excute();
	return 0;
}