#include "RasterizerImport.h"

namespace Engine::Resource::Import
{
	ComPtr<ID3DBlob> Engine::Resource::Import::RasterizerShader(const std::string& a_path)
	{
		ComPtr<ID3DBlob> _cpBlob = {};

		// 文字列変換
		std::wstring _wstr = StringUtility::ToWideString(a_path);

		// シェーダー読み込み
		auto _hr = D3DReadFileToBlob(
			_wstr.c_str(),
			_cpBlob.ReleaseAndGetAddressOf()
		);
		if (FAILED(_hr))
		{
			assert(0 && "シェーダーの読み込みに失敗");
			return _cpBlob;
		}

		return _cpBlob;
	}
	D3D12_SHADER_BYTECODE CreateShaderByteCode(ID3DBlob* a_pBlob)
	{
		return CD3DX12_SHADER_BYTECODE(a_pBlob);
	}
}
