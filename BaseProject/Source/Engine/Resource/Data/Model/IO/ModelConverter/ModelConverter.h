#pragma once

#include "../../../../Loader/Model/Importer/tinyGLTF/tinyGLTF.h"

namespace Engine::Resource::Converter
{
	class ModelConverter
	{
	public:
		ModelConverter() = default;
		~ModelConverter() = default;

		/// <summary>
		/// パースされたデータをエンジン側の仕様に合わせて詰め替え
		/// </summary>
		/// <param name="a_filePath">モデルパス</param>
		/// <param name="a_rawModel">パースされたモデルデータ</param>
		/// <returns>エンジン形式のモデルデータ</returns>
		static ModelData ConvertModelData(const std::string& a_filePath, const GLTF::ModelData& a_rawModel);

	private:

	};
}