#include "Shader.h"

#include "../../Loader/Shader/Importer/Rasterizer/RasterizerImport.h"

void Engine::Resource::Shader::Load(const std::string& a_path)
{
	m_cpBlob = Import::CompileShader(a_path);
	m_byteCode = Import::CreateShaderByteCode(m_cpBlob.Get());

	m_path = a_path;
}

const D3D12_SHADER_BYTECODE& Engine::Resource::Shader::GetByteCode() const
{
	return m_byteCode;
}
