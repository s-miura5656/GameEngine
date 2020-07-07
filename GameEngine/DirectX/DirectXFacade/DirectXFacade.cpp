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

	//! DirectX12�֘A������
	if (FAILED(InitializeDXGIDevice()))
	{
		assert(0);
		return;
	}
	
	//! �R�}���h�֘A������
	if (FAILED(CreateCommand()))
	{
		assert(0);
		return;
	}

	//! �X���b�v�`�F�C������
	if (FAILED(CreateSwapChain(hwnd)))
	{
		assert(0);
		return;
	}

	//! �����_�[�^�[�Q�b�g����
	if (FAILED(CreateFinalRTV()))
	{
		assert(0);
		return;
	}

	//! �t�F���X�̍쐬
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

	//! ���߂̃N���[�Y
	m_cmd_list->Close();

	//! �R�}���h���X�g�̎��s
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

	//! �L���[���N���A
	m_cmd_allocator->Reset();

	//! �R�}���h���X�g���N���A
	m_cmd_list->Reset(m_cmd_allocator.Get(), nullptr);

	//! �t���b�v
	m_swapchain->Present(1, 0);
}

//! @brief DXGI����̏�����
//! @remarks DirectX12����̏������A�Ώۂ̃A�_�v�^�[�������A
//! @remarks Direct3D�̏��������s���܂��B
HRESULT DirectXFacade::InitializeDXGIDevice()
{
	UINT flagsDXGI = DXGI_CREATE_FACTORY_DEBUG;

	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(m_dxgi_factory.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return result;
	}

	//! DirectX12�܂�菉����
	//! �t�B�[�`�����x����
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//! �A�_�v�^�[�̗񋓗p
	std::vector<ComPtr<IDXGIAdapter>> adapters;

	//! ����̃A�_�v�^�[�I�u�W�F�N�g�̓��ꕨ
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

	//! Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL feature_level;
	for (auto l : levels) {
		if (D3D12CreateDevice(tmp_adapter.Get(), l, IID_PPV_ARGS(m_dev.ReleaseAndGetAddressOf())) == S_OK) {
			feature_level = l;
			break;
		}
	}

	return result;
}

//! @brief �R�}���h�܂��̏�����
//! @remarks �R�}���h�A���P�[�^�[�A�R�}���h���X�g�A�R�}���h�L���[��
//! @remarks �����A���������܂��B
HRESULT DirectXFacade::CreateCommand()
{
	//! �R�}���h�A���P�[�^�[�̐���
	auto result = m_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmd_allocator.ReleaseAndGetAddressOf()));

	if (FAILED(result)) 
	{
		assert(0);
		return result;
	}

	//! �R�}���h���X�g�̐���
	result = m_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocator.Get(), nullptr, IID_PPV_ARGS(m_cmd_list.ReleaseAndGetAddressOf()));

	if (FAILED(result)) 
	{
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
	
	//! �^�C���A�E�g�Ȃ�
	cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		   
	//! �A�_�v�^�[�͂P�̎��͂O
	cmd_queue_desc.NodeMask = 0;
	//! �v���C�I���e�B���Ɏw��Ȃ�
	cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; 
	//! �����̓R�}���h���X�g�ƍ��킹�Ă�������
	cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//! �R�}���h�L���[�̐���
	result = m_dev->CreateCommandQueue(&cmd_queue_desc,
		IID_PPV_ARGS(m_cmd_queue.ReleaseAndGetAddressOf()));	   

	assert(SUCCEEDED(result));

	return result;
}

//! @brief �X���b�v�`�F�C���̐���
//! @remarks ���݂ɃX���b�v�����ʂ����܂�
HRESULT DirectXFacade::CreateSwapChain(const HWND& hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	
	//! ��ʉ𑜓x�i���j
	swapchainDesc.Width				 = m_window_size.cx;
	//! ��ʉ𑜓x�i�c�j
	swapchainDesc.Height			 = m_window_size.cy;
	//! �s�N�Z���t�H�[�}�b�g 
	swapchainDesc.Format			 = DXGI_FORMAT_R8G8B8A8_UNORM;
	//! �X�e���I�\���t���O�i3D �f�B�X�v���C�̃X�e���I���[�h�j
	swapchainDesc.Stereo			 = false;
	//! AA�i�A���`�G�C���A�X�j�ׂ̈̃T���v�����O�񐔂��w��BAA���g�p���Ȃ��ꍇ�A1���w��B
	swapchainDesc.SampleDesc.Count   = 1;
	//! �T���v�����O�̕i�����w��BAA���g�p���Ȃ��ꍇ�A0���w��B
	swapchainDesc.SampleDesc.Quality = 0;
	//! �t���[���o�b�t�@�̎g�p���@���w��
	swapchainDesc.BufferUsage        = DXGI_USAGE_BACK_BUFFER;
	//! �_�u���o�b�t�@�Ƃ��Ďg��
	swapchainDesc.BufferCount        = 2;
	//! �t���[���o�b�t�@�ƃE�B���h�E�̃T�C�Y���قȂ�ꍇ�̕\�����@���w��
	swapchainDesc.Scaling	         = DXGI_SCALING_STRETCH;
	//! ��ʂ�؂�ւ�����A�`���ʂɂȂ����t���[���o�b�t�@�̈��������w��
	swapchainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//! �t���[���o�b�t�@�̓��e���E�B���h�E�ɕ\������ۂ̃A���t�@�l�̈��������w��
	swapchainDesc.AlphaMode	         = DXGI_ALPHA_MODE_UNSPECIFIED;
	//! DXGI_SWAP_CHAIN_FLAG �񋓌^�̒l�̘_���a���w��B�ʏ�͎g��Ȃ��̂� 0 ���w��B
	swapchainDesc.Flags		         = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//! �X���b�v�`�F�C���̐���
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

	//! �f�X�N���v�^�q�[�v�̃^�C�v��ݒ�B�g���݂��ɉ�����4�̃^�C�v����ݒ�
	heapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV; 
	
	//! ������GPU���g�p����ꍇ�A�ǂ�GPU�����̃f�X�N���v�^�q�[�v�����w��B
	//! GPU��1�����g�p���Ȃ��ꍇ�A�O��ݒ�
	heapDesc.NodeMask		= 0;
	
	//! �i�[�ł���f�X�N���v�^�̐��i�f�X�N���v�^�q�[�v���z��ׁ̈A�z�񐔂̎w�肪�K�v�j�B
	//! �ʏ탌���_�[�^�[�Q�b�g�̓t���[���o�b�t�@��1�F1�ɂȂ�悤�ɍ쐬����
	heapDesc.NumDescriptors = 2;

	//! D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_NONE��ݒ�B
	//! �f�X�N���v�^���w���Ă���f�[�^���V�F�[�_����Q�Ƃ������ꍇ��
	//! D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE��ݒ�B
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
