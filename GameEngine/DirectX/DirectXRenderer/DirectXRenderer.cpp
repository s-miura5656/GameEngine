#include "DirectXRenderer.h"
#include "../DirectXFacade/DirectXFacade.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

DX12Renderer::DX12Renderer(std::shared_ptr<DirectXFacade> dx12) : m_directx_facade(dx12.get())
{
	CreateVertexBuffer();
	CreateVertexCopy();
	CompileShader();
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

//! @brief パイプラインステート生成
//! @param input_layout セマンティクスのパラメータ
//! @remarks パイプラインステートのパラメータを入力
HRESULT DX12Renderer::CreatePipeline(std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	CreateRootSignature();

	//! ルートシグネチャ
	gpipeline.pRootSignature = m_root_signature.Get();
	
	//! シェーダー
	gpipeline.VS.pShaderBytecode = m_vs_blob->GetBufferPointer();
	gpipeline.VS.BytecodeLength  = m_vs_blob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = m_ps_blob->GetBufferPointer();
	gpipeline.PS.BytecodeLength  = m_ps_blob->GetBufferSize();

	//! サンプルマスク
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//! ブレンドステート
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	//! ラスタライザー
	gpipeline.RasterizerState.MultisampleEnable = false;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeline.RasterizerState.DepthClipEnable = true;

	//! 深度
	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファーを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	//! 入力レイアウト
	gpipeline.InputLayout.pInputElementDescs = &input_layout[0];
	gpipeline.InputLayout.NumElements = sizeof(input_layout) / sizeof(input_layout[0]);
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//! レンダーターゲットの設定
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//! AAの為のサンプル数の設定
	gpipeline.SampleDesc.Count = 1;
	gpipeline.SampleDesc.Quality = 0;

	//! パイプラインステートオブジェクトの生成
	auto result = m_directx_facade->GetDevice()->CreateGraphicsPipelineState(&gpipeline,
									IID_PPV_ARGS(m_pipelinestate.ReleaseAndGetAddressOf()));

	return result;
}

//! @brief ルートシグネチャ生成
//! @remarks ルートシグネチャのパラメータ入力
HRESULT DX12Renderer::CreateRootSignature()
{
	D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};

	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> root_sig_blob = nullptr;

	auto result = D3D12SerializeRootSignature(
				  &root_signature_desc,
				  D3D_ROOT_SIGNATURE_VERSION_1_0,
				  &root_sig_blob,
				  &m_error_blob);

	result = m_directx_facade->GetDevice()->CreateRootSignature(
											0,
											root_sig_blob->GetBufferPointer(),
											root_sig_blob->GetBufferSize(),
											IID_PPV_ARGS(m_root_signature.ReleaseAndGetAddressOf()));

	root_sig_blob->Release();

	return result;
}

//! @brif シェーダーのコンパイル
HRESULT DX12Renderer::CompileShader()
{
	auto result = D3DCompileFromFile(L"Shader/Default/BasicVertexShader.hlsl",
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

	result = D3DCompileFromFile(L"Shader/Default/BasicPixelShader.hlsl",
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

	std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
					 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//					 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//					 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"BONE_NO",  0, DXGI_FORMAT_R16G16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//					 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"WEIGHT",   0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//					 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"EDGEFLT",  0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//					 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	//! パイプラインステート生成
	CreatePipeline(input_layout);

	return result;
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



