#pragma once

#include "../Parser/tinyGLTF/tinyGLTF.h"

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

		/// <summary>
		/// 指定したモデルファイルをbinary、DDSなどに変換したファイルを作る。
		/// </summary>
		static bool ConvertModelDataToBinary(const std::string& a_filePath);		// ファイルパスから
		static bool ConvertModelDataToBinary(const Engine::GUID& a_guid);			// guidから
		static bool ConvertModelDataToBinary(const ResourceRef<Model>& a_modelHandle);	// ハンドルから

	private:

		// binaryにコンバート
		static void ConvertMaterialToBinary(const std::string& a_basePath,ModelAssetData& a_asset,const ModelRuntimeData& a_runtime);
		static void ConvertMeshToBinary(const std::string& a_basePath,ModelAssetData& a_asset,const ModelRuntimeData& a_runtime);
		static void ConvertAnimationToBinary(const std::string& a_basePath,ModelAssetData& a_asset,const ModelRuntimeData& a_runtime);


		static void ConvertTexture(const ResourceRef<Texture>& a_ref);
		static Texture* GetTexture(const ResourceRef<Texture>& a_ref);

	};
}