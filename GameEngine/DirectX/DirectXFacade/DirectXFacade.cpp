#include "DirectXFacade.h"
#include "../../Application/Application.h"
#include "../../../DirectXTex-master/DirectXTex/d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

DirectXFacade::DirectXFacade(HWND& hwnd)
{
#ifdef _DEBUG
	DirectXHelper::Instance().EnableDebugLayer();
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, 
					   IID_PPV_ARGS(m_dxgi_factory.ReleaseAndGetAddressOf()));
#else
	CreateDXGIFactory1(IID_PPV_ARGS(m_dxgi_factory.ReleaseAndGetAddressOf()));
#endif // _DEBUG

	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);

	auto& app = Application::Instance();

	m_window_size = app.GetWindowSize();

	//! DirectX12関連初期化
	if (FAILED(InitializeDXGIDevice()))
	{
		assert(0);
		return;
	}
	
	//! コマンド関連初期化
	if (FAILED(CreateCommand()))
	{
		assert(0);
		return;
	}

	//! スワップチェイン生成
	if (FAILED(CreateSwapChain(hwnd)))
	{
		assert(0);
		return;
	}

	//! レンダーターゲット生成
	if (FAILED(CreateFinalRTV()))
	{
		assert(0);
		return;
	}

	//! フェンスの作成
	if (FAILED(m_dev->CreateFence(m_fence_val, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()))))
	{
		assert(0);
		return;
	}
}

void DirectXFacade::BeginDraw()
{
	auto bbIdx = m_swapchain->GetCurrentBackBufferIndex();

	auto rtv_h = m_rtv_heaps->GetCPUDescriptorHandleForHeapStart();

	rtv_h.ptr += bbIdx * m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cmd_list->OMSetRenderTargets(1, &rtv_h, true, nullptr);

	m_cmd_list->ClearRenderTargetView(rtv_h, clear_color, 0, nullptr);
}

void DirectXFacade::SetScene()
{

}

void DirectXFacade::EndDraw()
{
	CreateResouceBarrier();

	//! 命令のクローズ
	m_cmd_list->Close();

	//! コマンドリストの実行
	ID3D12CommandList* cmd_lists[] = { m_cmd_list.Get() };
	m_cmd_queue->ExecuteCommandLists(1, cmd_lists);

	m_cmd_queue->Signal(m_fence.Get(), ++m_fence_val);

	if (m_fence->GetCompletedValue() != m_fence_val)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);

		m_fence->SetEventOnCompletion(m_fence_val, event);

		WaitForSingleObject(event, INFINITE);

		CloseHandle(event);
	}

	//! キューをクリア
	m_cmd_allocator->Reset();

	//! コマンドリストをクリア
	m_cmd_list->Reset(m_cmd_allocator.Get(), nullptr);

	//! フリップ
	m_swapchain->Present(1, 0);
}

//! @brief DXGI周りの初期化
//! @remarks DirectX12周りの初期化、対象のアダプターを検索、
//! @remarks Direct3Dの初期化を行います。
HRESULT DirectXFacade::InitializeDXGIDevice()
{
	UINT flagsDXGI = DXGI_CREATE_FACTORY_DEBUG;

	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(m_dxgi_factory.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return result;
	}

	//! DirectX12まわり初期化
	//! フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//! アダプターの列挙用
	std::vector<ComPtr<IDXGIAdapter>> adapters;

	//! 特定のアダプターオブジェクトの入れ物
	ComPtr<IDXGIAdapter> tmp_adapter = nullptr;

	for (int i = 0; m_dxgi_factory->EnumAdapters(i, &tmp_adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmp_adapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};

		adpt->GetDesc(&adesc);

		std::wstring strDesc = adesc.Description;

		if (strDesc.find(L"NVIDIA") != std::string::npos) {

			tmp_adapter = adpt;
			break;
		}
	}

	//! Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL feature_level;
	for (auto l : levels) {
		if (D3D12CreateDevice(tmp_adapter.Get(), l, IID_PPV_ARGS(m_dev.ReleaseAndGetAddressOf())) == S_OK) {
			feature_level = l;
			break;
		}
	}

	return result;
}

//! @brief コマンドまわりの初期化
//! @remarks コマンドアロケーター、コマンドリスト、コマンドキューを
//! @remarks 生成、初期化します。
HRESULT DirectXFacade::CreateCommand()
{
	//! コマンドアロケーターの生成
	auto result = m_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmd_allocator.ReleaseAndGetAddressOf()));

	if (FAILED(result)) 
	{
		assert(0);
		return result;
	}

	//! コマンドリストの生成
	result = m_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocator.Get(), nullptr, IID_PPV_ARGS(m_cmd_list.ReleaseAndGetAddressOf()));

	if (FAILED(result)) 
	{
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
	
	//! タイムアウトなし
	cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		   
	//! アダプターは１つの時は０
	cmd_queue_desc.NodeMask = 0;
	//! プライオリティ特に指定なし
	cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; 
	//! ここはコマンドリストと合わせてください
	cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//! コマンドキューの生成
	result = m_dev->CreateCommandQueue(&cmd_queue_desc,
		IID_PPV_ARGS(m_cmd_queue.ReleaseAndGetAddressOf()));	   

	assert(SUCCEEDED(result));

	return result;
}

//! @brief スワップチェインの生成
//! @remarks 交互にスワップする画面を作ります
HRESULT DirectXFacade::CreateSwapChain(const HWND& hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	
	//! 画面解像度（横）
	swapchainDesc.Width				 = m_window_size.cx;
	//! 画面解像度（縦）
	swapchainDesc.Height			 = m_window_size.cy;
	//! ピクセルフォーマット 
	swapchainDesc.Format			 = DXGI_FORMAT_R8G8B8A8_UNORM;
	//! ステレオ表示フラグ（3D ディスプレイのステレオモード）
	swapchainDesc.Stereo			 = false;
	//! AA（アンチエイリアス）の為のサンプリング回数を指定。AAを使用しない場合、1を指定。
	swapchainDesc.SampleDesc.Count   = 1;
	//! サンプリングの品質を指定。AAを使用しない場合、0を指定。
	swapchainDesc.SampleDesc.Quality = 0;
	//! フレームバッファの使用方法を指定
	swapchainDesc.BufferUsage        = DXGI_USAGE_BACK_BUFFER;
	//! ダブルバッファとして使う
	swapchainDesc.BufferCount        = 2;
	//! フレームバッファとウィンドウのサイズが異なる場合の表示方法を指定
	swapchainDesc.Scaling	         = DXGI_SCALING_STRETCH;
	//! 画面を切り替えた後、描画画面になったフレームバッファの扱い方を指定
	swapchainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//! フレームバッファの内容をウィンドウに表示する際のアルファ値の扱い方を指定
	swapchainDesc.AlphaMode	         = DXGI_ALPHA_MODE_UNSPECIFIED;
	//! DXGI_SWAP_CHAIN_FLAG 列挙型の値の論理和を指定。通常は使わないので 0 を指定。
	swapchainDesc.Flags		         = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//! スワップチェインの生成
	auto result = m_dxgi_factory->CreateSwapChainForHwnd(
				  m_cmd_queue.Get(),
				  hwnd,
				  &swapchainDesc,
				  nullptr,
				  nullptr,
				  (IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf());

	assert(SUCCEEDED(result));

	return result;
}

HRESULT DirectXFacade::CreateFinalRTV()
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};

	auto result = m_swapchain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	//! デスクリプタヒープのタイプを設定。使いみちに応じて4つのタイプから設定
	heapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV; 
	
	//! 複数のGPUを使用する場合、どのGPU向けのデスクリプタヒープかを指定。
	//! GPUを1つしか使用しない場合、０を設定
	heapDesc.NodeMask		= 0;
	
	//! 格納できるデスクリプタの数（デスクリプタヒープが配列の為、配列数の指定が必要）。
	//! 通常レンダーターゲットはフレームバッファと1：1になるように作成する
	heapDesc.NumDescriptors = 2;

	//! D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_NONEを設定。
	//! デスクリプタが指しているデータをシェーダから参照したい場合は
	//! D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLEを設定。
	heapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtv_heaps));

	if (FAILED(result))
	{
		SUCCEEDED(result);
		return result;
	}

	DXGI_SWAP_CHAIN_DESC swcDesc = {};

	result = m_swapchain->GetDesc(&swcDesc);

	m_back_buffers.resize(swcDesc.BufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtv_heaps->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_back_buffers[i]));
		
		assert(SUCCEEDED(result));
		
		rtvDesc.Format = m_back_buffers[i]->GetDesc().Format;
		
		m_dev->CreateRenderTargetView(m_back_buffers[i].Get(), &rtvDesc, handle);
		
		handle.ptr += m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//m_viewport.reset(new CD3DX12_VIEWPORT(_backBuffers[0]));
	//m_scissorrect.reset(new CD3DX12_RECT(0, 0, desc.Width, desc.Height));

	return result;
}



void DirectXFacade::CreateResouceBarrier()
{
	auto bb_idx = m_swapchain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier_desc = {};

	barrier_desc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier_desc.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier_desc.Transition.pResource   = m_back_buffers[bb_idx].Get();
	barrier_desc.Transition.Subresource = 0;
	barrier_desc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier_desc.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

	m_cmd_list->ResourceBarrier(1, &barrier_desc);
}
