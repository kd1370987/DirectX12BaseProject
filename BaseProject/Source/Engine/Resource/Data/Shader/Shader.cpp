#include "Shader.h"

#include "../../Importer/Shader/Rasterizer/RasterizerImport.h"

void Engine::Resource::Shader::Load(const std::string& a_path)
{
	auto _data = Import::RasterizerShader(a_path);

	m_cpBlob = _data.cpBlob;
	m_byteCode = _data.byteCode;
}

D3D12_SHADER_BYTECODE Engine::Resource::Shader::GetByteCode()
{
	return m_byteCode;
}
