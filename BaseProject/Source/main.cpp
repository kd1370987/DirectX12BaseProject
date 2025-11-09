#include "Engine/Core/App.h"

int wmain(int argc, wchar_t** argv,wchar_t** envp)
{
	printf("ハローワールド\n");
	
	// 起動直後（Device作成の前）に一度だけ
#if defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	StartApp(TEXT("DirectX12入門"));
	return 0;
}