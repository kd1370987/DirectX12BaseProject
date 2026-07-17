#pragma once

namespace Engine::Resource
{
	namespace Import
	{
		ComPtr<IDxcBlob> DXIL(const std::string& a_path);
	}
}