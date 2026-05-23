#pragma once
namespace Engine::Resource
{
	// 独自形式のモデルを読み込む
	bool Load(const std::string& a_filePath, ModelData& a_outData);
}