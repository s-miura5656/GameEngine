#include "Application/Application.h"
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#ifdef _DEBUG
int main()
{
#else
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif // _DEBUG
	
	auto& app = Application::Instance();

	if (!app.Initialize())
	{
		return -1;
	}

	app.Run();

	app.Finalize();

	return 0;
}