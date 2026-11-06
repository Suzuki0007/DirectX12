module;
#include <tchar.h>
#include <Windows.h>
module win32application;

namespace cw = createWindow;

Win32Application* Win32Application::GetInstance()
{
	static Win32Application instance;
	return &instance;
}

void Win32Application::Run(DXApplication* dxApp,HINSTANCE hInstance)
{
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);// ウィンドウクラスのサイズを指定(バージョンの判別などに使う)
	windowClass.style = CS_HREDRAW | CS_VREDRAW;// ウィンドウのサイズが変更されたときに再描画する
	windowClass.lpfnWndProc = WindowProc;// 関数の場所を代入する
	windowClass.hInstance = hInstance;// このウィンドウの持ち主はどのプログラムかを指定する
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);// マウスポインタを矢印に設定
	windowClass.lpszClassName = _T("DXSampleClass");// ウィンドウクラスの名前を指定する
	RegisterClassEx(&windowClass);// ウィンドウクラスを登録する

	// ウィンドウのサイズを指定する
	RECT windowRect = { win32::WindowX, win32::WindowY, static_cast<LONG>(dxApp->GetWidth()), static_cast<LONG>(dxApp->GetHeight())};
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウを作成する
	HWND hwnd = CreateWindowEx
	(
		cw::alpha,
		windowClass.lpszClassName,// ウィンドウクラスの名前を指定する
		dxApp->GetTitle(),// ウィンドウのタイトルを指定する
		WS_OVERLAPPEDWINDOW,// ウィンドウのスタイルを指定する
		CW_USEDEFAULT,// ウィンドウのX座標を指定する
		CW_USEDEFAULT,// ウィンドウのY座標を指定する
		windowRect.right - windowRect.left,// ウィンドウの幅を指定する
		windowRect.bottom - windowRect.top,// ウィンドウの高さを指定する
		nullptr,// 親ウィンドウを指定する
		nullptr,// メニューハンドルを指定する
		hInstance,// このウィンドウの持ち主はどのプログラムかを指定する
		nullptr// ウィンドウ作成データを指定する
	);

	// アプリケーションの初期化
	dxApp->Initialize(hwnd);

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	// メインループ
	MSG msg = {};
	//アプリ終了までループする
	while(msg.message != WM_QUIT)
	{
		// メッセージがあるか確認する
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);// キーボードのメッセージを文字コードに変換する
			DispatchMessage(&msg);// メッセージを処理関数に送る
		}

		// アプリの処理
		dxApp->Process();
		dxApp->Render();
	}
	
	// アプリケーションの終了
	dxApp->Terminate();

	// クラスを登録解除する
	UnregisterClass(windowClass.lpszClassName, hInstance);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);// アプリ終了のメッセージを送る
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, message, wparam, lparam);// デフォルトの処理関数を呼び出す
		}
	}
	return 0;
}