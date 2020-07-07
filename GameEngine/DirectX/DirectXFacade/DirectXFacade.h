#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <map>
#include <unordered_map>
#include <wrl.h>
#include <string>
#include <functional>
#include <memory>
#include <tchar.h>
#include <DirectXMath.h>
#include "../DirectXHelper/DirectXHelper.h"

class DirectXFacade
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	const float clear_color[4] = { 1.f, 1.f, 0.f, 1.f };

public:
	

	DirectXFacade(HWND& hwnd);
	~DirectXFacade() {
		m_fence->Release();
		m_rtv_heaps->Release();
		m_cmd_queue->Release();
		m_cmd_list->Release();
		m_cmd_allocator->Release();
		m_swapchain->Release();
		m_dxgi_factory->Release();
		m_dev->Release();
	};

	HRESULT InitializeDXGIDevice();
	HRESULT CreateCommand();
	HRESULT CreateSwapChain(const HWND& hwnd);
	HRESULT CreateFinalRTV();

	void BeginDraw();
	void SetScene();
	void EndDraw();

	void CreateResouceBarrier();

	inline ComPtr<ID3D12Device> GetDevice() { return m_dev; }
	inline ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_cmd_list; }
	inline ComPtr<IDXGISwapChain4> GetSwapchain() { return m_swapchain; }

private:
	ComPtr<ID3D12Device> m_dev = nullptr;
	ComPtr<IDXGIFactory6> m_dxgi_factory = nullptr;
	ComPtr<IDXGISwapChain4> m_swapchain = nullptr;
	ComPtr<ID3D12CommandAllocator> m_cmd_allocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> m_cmd_list = nullptr;
	ComPtr<ID3D12CommandQueue> m_cmd_queue = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_rtv_heaps = nullptr;
	ComPtr<ID3D12Fence> m_fence = nullptr;
	UINT m_fence_val;
	std::vector<ComPtr<ID3D12Resource>> m_back_buffers = { nullptr };
	std::unique_ptr<D3D12_VIEWPORT> m_viewport = nullptr;
	std::unique_ptr<D3D12_RECT> m_scissorrect = nullptr;
	SIZE m_window_size;
};