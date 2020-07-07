#include "DirectXRenderer.h"
#include "../DirectXFacade/DirectXFacade.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler,lib")

DX12Renderer::DX12Renderer(std::shared_ptr<DirectXFacade> dx12) : m_directx_facade(dx12.get())
{
	CreateVertexBuffer();
	CreateVertexCopy();
}

void DX12Renderer::Draw()
{
	CreateVertexView();
}

//! @brief 頂点バッファーの生成
void DX12Renderer::CreateVertexBuffer()
{
	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width            = sizeof(vertices);
	resdesc.Height           = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels        = 1;
	resdesc.Format           = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags            = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	auto result = m_directx_facade->GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vert_buff));
}

//! @brief 頂点情報のコピー
//! @remarks 
void DX12Renderer::CreateVertexCopy()
{
	auto result = m_vert_buff->Map(0, nullptr, (void**)&m_vert_map);

	std::copy(std::begin(vertices), std::end(vertices), m_vert_map);

	m_vert_buff->Unmap(0, nullptr);
}

//! @brief 頂点バッファビュー作成
void DX12Renderer::CreateVertexView()
{
	m_vbview.BufferLocation = m_vert_buff->GetGPUVirtualAddress();
	m_vbview.SizeInBytes    = sizeof(vertices);
	m_vbview.StrideInBytes  = sizeof(vertices[0]);

	m_directx_facade->GetCommandList()->IASetVertexBuffers(0, 1, &m_vbview);
}

//! @brif シェーダーコンパイル
HRESULT DX12Renderer::CompileShader()
{
	auto result = D3DCompileFromFile(L"Shader/BasicVertexShader.hlsl",
									 nullptr,
									 D3D_COMPILE_STANDARD_FILE_INCLUDE,
									 "BasicVS", "vs_5_0",
									 D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
									 0, &m_vs_blob, &m_error_blob);

	if (!CheckShaderCompileResult(result, m_error_blob.Get()))
	{
		assert(0);
		return result;
	}

	result = D3DCompileFromFile(L"Shader/BasicPixelShader.hlsl",
								nullptr,
								D3D_COMPILE_STANDARD_FILE_INCLUDE,
								"BasicPS", "ps_5_0",
								D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
								0, &m_ps_blob, &m_error_blob);

	if (!CheckShaderCompileResult(result, m_error_blob.Get()))
	{
		assert(0);
		return result;
	}
}

bool DX12Renderer::CheckShaderCompileResult(HRESULT result, ComPtr<ID3DBlob> error)
{
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA(" ファイルが見当たりません ");
		}
		else
		{
			std::string errstr;
			errstr.resize(error->GetBufferSize());
			std::copy_n((char*)error->GetBufferPointer(),
				error->GetBufferSize(),
				errstr.begin());
			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
		return false;
	}
	else
	{
		return true;
	}
}



