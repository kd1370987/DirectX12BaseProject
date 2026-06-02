#include "ComputePass.h"

#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics
{
	void ComputePass::SetName(const std::string& a_name)
	{
		m_name = a_name;
		m_csPSODesc.SetName(a_name);
	}
	void ComputePass::SetShader(const std::string& a_filePath)
	{
		auto _csHandle = Resource::ShaderLoader::Request(a_filePath);
		const auto& _byteCode = Resource::ResourceManager::Instance().Get(_csHandle)->GetByteCode();
		m_csPSODesc.SetCS(_byteCode);
	}
}