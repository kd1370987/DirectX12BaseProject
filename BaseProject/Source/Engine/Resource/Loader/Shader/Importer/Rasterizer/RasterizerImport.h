#pragma once

namespace Engine::Resource
{
	namespace Import
	{
		// ブロッブ作成
		ComPtr<ID3DBlob> CompileShader(const std::string& a_path);

		// バイトコード作成
		D3D12_SHADER_BYTECODE CreateShaderByteCode(ID3DBlob* a_pBlob);

	}
}