#include "Application.h"

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) 
{
	//! ウィンドウが破棄されたら呼ばれます
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	//! 規定の処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool Application::Initialize()
{
	CreateGameWindow(m_hwnd, m_windowClass);

	m_directx_facade.reset(new DirectXFacade(m_hwnd));
	m_dx12_renderer.reset(new DX12Renderer(m_directx_facade));

	// ウィンドウ表示
	ShowWindow(m_hwnd, SW_SHOW);

	return true;
}

void Application::Run()
{
	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときに message が WM_QUIT になる
		if (msg.message == WM_QUIT)
		{
			break;
		}

		m_directx_facade->BeginDraw();

		m_directx_facade->SetScene();
		m_directx_facade->EndDraw();
	}
}

void Application::Finalize()
{
	m_directx_facade.reset();

	UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
}

SIZE Application::GetWindowSize() const
{
	SIZE ret;
	ret.cx = window_width;
	ret.cy = window_height;
	return ret;
}

//! @brief ウィンドウ生成
//! @param hwnd = ウィンドウハンドル
//! @param window_class = ウィンドウクラス
//! @remarks ゲームウィンドウを作ります。
void Application::CreateGameWindow(HWND& hwnd, WNDCLASSEX& window_class)
{
	DirectXHelper::Instance().DebugOutputFormatString("Show window test.");

	HINSTANCE hInst = GetModuleHandle(nullptr);

	//! ウィンドウクラス生成＆登録
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	window_class.lpszClassName = _T("DirectXTest");      // アプリケーションクラス名(適当でいいです)
	window_class.hInstance = GetModuleHandle(0);         // ハンドルの取得

	//! アプリケーションクラス(こういうの作るからよろしくってOSに予告する)
	RegisterClassEx(&window_class);

	RECT wrc = { 0, 0, window_width, window_height };    // ウィンドウサイズを決める

	//! ウィンドウのサイズ補正
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//! ウィンドウオブジェクト生成
	m_hwnd = CreateWindow(window_class.lpszClassName, //! クラス名指定
			 _T("DX12 Test"),						  //! タイトルバーの文字
			 WS_OVERLAPPEDWINDOW,					  //! タイトルバーと境界線があるウィンドウです
			 CW_USEDEFAULT,							  //! 表示 X 座標はOSにお任せします
			 CW_USEDEFAULT,							  //! 表示 Y 座標はOSにお任せします
			 wrc.right - wrc.left,					  //! ウィンドウ幅
			 wrc.bottom - wrc.top,					  //! ウィンドウ高
			 nullptr,								  //! 親ウィンドウハンドル
			 nullptr,								  //! メニューハンドル
			 window_class.hInstance,				  //! 呼び出しアプリケーションハンドル
			 nullptr);								  //! 追加パラメータ
}
