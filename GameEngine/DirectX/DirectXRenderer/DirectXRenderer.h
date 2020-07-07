#pragma once

#include<d3d12.h>
#include<vector>
#include<wrl.h>
#include<memory>
#include <DirectXMath.h>

class DirectXFacade;

class DX12Renderer
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	DirectX::XMFLOAT3 vertices[3] =
	{
		{-1.0f, -1.0f, 0.0f },
		{-1.0f,  1.0f, 0.0f },
		{ 1.0f, -1.0f, 0.0f },
	};

public:
	DX12Renderer(std::shared_ptr<DirectXFacade> dx12);
	~DX12Renderer() {};

	void Draw();

	void CreateVertexBuffer();
	void CreateVertexCopy();
	void CreateVertexView();
	HRESULT CompileShader();
	bool CheckShaderCompileResult(HRESULT result, ComPtr<ID3DBlob> error);
private:
	std::unique_ptr<DirectXFacade> m_directx_facade = nullptr;
	ComPtr<ID3D12RootSignature> m_root_signature = nullptr;
	ComPtr<ID3D12PipelineState> m_pipeline_state = nullptr;
	ComPtr<ID3D12Resource> m_vert_buff = nullptr;
	DirectX::XMFLOAT3* m_vert_map = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vbview = {};
	ComPtr<ID3DBlob> m_vs_blob = nullptr;
	ComPtr<ID3DBlob> m_ps_blob = nullptr;
	ComPtr<ID3DBlob> m_error_blob = nullptr;
};