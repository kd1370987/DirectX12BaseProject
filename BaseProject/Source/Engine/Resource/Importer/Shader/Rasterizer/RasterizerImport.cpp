#include "RasterizerImport.h"

namespace Engine::Resource::Import
{
	ShaderData Engine::Resource::Import::RasterizerShader(const std::string& a_path)
	{
		ShaderData _data = {};

		// 文字列変換
		std::wstring _wstr = StringUtility::ToWideString(a_path);

		// シェーダー読み込み
		auto _hr = D3DReadFileToBlob(
			_wstr.c_str(),
			_data.cpBlob.ReleaseAndGetAddressOf()
		);
		if (FAILED(_hr))
		{
			assert(0 && "シェーダーの読み込みに失敗");
			return _data;
		}

		// バイトデータからコードを生成
		_data.byteCode = D3D12_SHADER_BYTECODE(_data.cpBlob.Get());

		return _data;
	}
}
