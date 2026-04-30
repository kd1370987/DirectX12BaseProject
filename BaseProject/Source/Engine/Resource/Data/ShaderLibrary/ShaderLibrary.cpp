#include "ShaderLibrary.h"

#include "../../Loader/Shader/Importer/DXIL/DXIL.h"

void Engine::Resource::ShaderLibrary::Load(const std::string& a_path)
{
	m_cpIDxcBlob = Import::DXIL(a_path);
}

IDxcBlob* Engine::Resource::ShaderLibrary::GetIDxcBlob()
{
	return m_cpIDxcBlob.Get();
}
