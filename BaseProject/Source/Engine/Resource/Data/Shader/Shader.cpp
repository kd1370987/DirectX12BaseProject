#include "Shader.h"

#include "../../Loader/Shader/Importer/Rasterizer/RasterizerImport.h"

void Engine::Resource::Shader::Load(const std::string& a_path, std::vector<LPCWSTR> a_setting, const wchar_t* a_version)
{
	std::vector<LPCWSTR> _setting = {};
	if(a_setting.empty())
	{
		_setting = {
			L"-I", L"Asset\\Shader\\Ray",
			L"-I", L"Asset\\Shader\\Source\\Mesh", 
			L"-I", L"Asset\\Shader\\Source\\DebugLineShader",
			L"-I", L"Asset\\Shader\\Common",
			L"-I", L"Asset\\Shader\\Common\\CB",
			L"-I", L"Asset\\Shader\\Common\\Math",
			L"-I", L"Asset\\Shader\\Common\\RootParameters",
			L"-I", L"Asset\\Shader\\Common\\Shape",
			L"-I", L"Asset\\Shader\\Common\\Struct",
			L"-Zi",
			L"-Qembed_debug"
		};
	}
	else
	{
		_setting = a_setting;
	}
	m_cpBlob = RequestShader(a_path, a_version, _setting);
	m_byteCode = Import::CreateShaderByteCode(m_cpBlob.Get());
	//m_stage = Import::ReflectShaderStage(a_path);
	m_path = a_path;
}

void Engine::Resource::Shader::Release()
{
	m_cpBlob.Reset();
	m_byteCode = {};
	m_path.clear();
}

const D3D12_SHADER_BYTECODE& Engine::Resource::Shader::GetByteCode() const
{
	return m_byteCode;
}

ID3DBlob* Engine::Resource::Shader::Get()
{
	return m_cpBlob.Get();
}
