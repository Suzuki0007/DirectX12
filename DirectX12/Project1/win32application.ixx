module;
#include <Windows.h>

export module win32application;
import dxapplication;

export namespace win32
{
	constexpr int WindowX = 0;
	constexpr int WindowY = 0;
	constexpr int WindowW = 1280;
	constexpr int WindowH = 720;
}

export namespace createWindow
{
	constexpr int alpha = 0;
}

export class Win32Application
{
public:
	static Win32Application* GetInstance();

	void Run(DXApplication* dxApp, HINSTANCE hInstance);

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
};