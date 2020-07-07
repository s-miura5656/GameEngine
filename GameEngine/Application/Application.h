#pragma once

#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "../DirectX/DirectXHelper/DirectXHelper.h"
#include "../DirectX/DirectXFacade/DirectXFacade.h"
#include "../DirectX/DirectXRenderer/DirectXRenderer.h"

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

class Application
{
public:
	~Application() {};

	bool Initialize();

	void Run();

	void Finalize();

	//! @brief インスタンス生成
	//! @remarks Application のシングルトンインスタンスを生成します。
	static Application& Instance() {
		static Application instance;
		return instance;
	};

	SIZE GetWindowSize() const;

private:
	Application() {};
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;

	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);

	WNDCLASSEX m_windowClass = {};
	HWND m_hwnd = {};

	std::shared_ptr<DirectXFacade> m_directx_facade;
	std::unique_ptr<DX12Renderer> m_dx12_renderer;
};