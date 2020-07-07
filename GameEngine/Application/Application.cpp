#include "Application.h"

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) 
{
	//! �E�B���h�E���j�����ꂽ��Ă΂�܂�
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	//! �K��̏������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool Application::Initialize()
{
	CreateGameWindow(m_hwnd, m_windowClass);

	m_directx_facade.reset(new DirectXFacade(m_hwnd));
	m_dx12_renderer.reset(new DX12Renderer(m_directx_facade));

	// �E�B���h�E�\��
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

		// �A�v���P�[�V�������I���Ƃ��� message �� WM_QUIT �ɂȂ�
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

//! @brief �E�B���h�E����
//! @param hwnd = �E�B���h�E�n���h��
//! @param window_class = �E�B���h�E�N���X
//! @remarks �Q�[���E�B���h�E�����܂��B
void Application::CreateGameWindow(HWND& hwnd, WNDCLASSEX& window_class)
{
	DirectXHelper::Instance().DebugOutputFormatString("Show window test.");

	HINSTANCE hInst = GetModuleHandle(nullptr);

	//! �E�B���h�E�N���X�������o�^
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[���o�b�N�֐��̎w��
	window_class.lpszClassName = _T("DirectXTest");      // �A�v���P�[�V�����N���X��(�K���ł����ł�)
	window_class.hInstance = GetModuleHandle(0);         // �n���h���̎擾

	//! �A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)
	RegisterClassEx(&window_class);

	RECT wrc = { 0, 0, window_width, window_height };    // �E�B���h�E�T�C�Y�����߂�

	//! �E�B���h�E�̃T�C�Y�␳
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//! �E�B���h�E�I�u�W�F�N�g����
	m_hwnd = CreateWindow(window_class.lpszClassName, //! �N���X���w��
			 _T("DX12 Test"),						  //! �^�C�g���o�[�̕���
			 WS_OVERLAPPEDWINDOW,					  //! �^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
			 CW_USEDEFAULT,							  //! �\�� X ���W��OS�ɂ��C�����܂�
			 CW_USEDEFAULT,							  //! �\�� Y ���W��OS�ɂ��C�����܂�
			 wrc.right - wrc.left,					  //! �E�B���h�E��
			 wrc.bottom - wrc.top,					  //! �E�B���h�E��
			 nullptr,								  //! �e�E�B���h�E�n���h��
			 nullptr,								  //! ���j���[�n���h��
			 window_class.hInstance,				  //! �Ăяo���A�v���P�[�V�����n���h��
			 nullptr);								  //! �ǉ��p�����[�^
}
