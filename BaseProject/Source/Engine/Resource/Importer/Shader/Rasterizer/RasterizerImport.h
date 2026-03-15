#pragma once

namespace Engine::Resource
{
	namespace Import
	{
		struct ShaderData
		{
			ComPtr<ID3DBlob> cpBlob;
			D3D12_SHADER_BYTECODE byteCode;
		};

		ShaderData RasterizerShader(const std::string& a_path);
	}
}