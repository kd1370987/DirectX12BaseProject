#include "ShaderLibrary.h"

#include "../../Loader/Shader/Importer/DXIL/DXIL.h"

void Engine::Resource::ShaderLibrary::Load(const std::string& a_path)
{
	m_cpIDxcBlob = Import::DXIL(a_path);
}

void Engine::Resource::ShaderLibrary::Release()
{
	m_cpIDxcBlob.Reset();
}

IDxcBlob* Engine::Resource::ShaderLibrary::GetIDxcBlob()
{
	return m_cpIDxcBlob.Get();
}
