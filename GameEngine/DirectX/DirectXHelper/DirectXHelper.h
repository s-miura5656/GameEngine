#pragma once

#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

class DirectXHelper
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	~DirectXHelper() {};

	void DebugOutputFormatString(const char* format, ...);
	void EnableDebugLayer();

	static DirectXHelper& Instance() 
	{
		static DirectXHelper instance;
		return instance;
	}

private:
	DirectXHelper() {};
	DirectXHelper(const DirectXHelper&) = delete;
	void operator=(const DirectXHelper&) = delete;
};