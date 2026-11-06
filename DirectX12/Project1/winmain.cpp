#include <Windows.h>
import win32application;
import dxapplication;

namespace Size
{
	constexpr unsigned int Width = 1280;
	constexpr unsigned int Height = 720;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	//Win32Application::GetInstance()->Run(hInstance);
	DXApplication dxApp(Size::Width, Size::Height, L"DirectX Sample");
	Win32Application::GetInstance()->Run(&dxApp, hInstance);
	return 0;
}