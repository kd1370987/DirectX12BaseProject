#pragma once

#include "../Parser/ParserStruct.h"

namespace Engine::Resource
{
	class ResourceManager;
}

namespace Engine::Resource::Converter
{
	class ModelConverter
	{
	public:
		ModelConverter() = default;
		~ModelConverter() = default;

		/// <summary>
		/// 加工済みの中間素材をエンジン側の仕様に合わせて詰め替え
		/// </summary>
		/// <param name="a_ctx">ビルドコンテキスト : GPUリソースの構築先</param>
		/// <param name="a_filePath">モデルパス</param>
		/// <param name="a_rawModel">加工済みの中間素材</param>
		/// <returns>エンジン形式のモデルデータ</returns>
		static ModelData ConvertModelData(
			const ResourceBuildContext& a_ctx,
			const std::string& a_filePath,
			const Parse::RawModel& a_rawModel
		);

		/// <summary>
		/// 指定したモデルファイルをbinary、DDSなどに変換したファイルを作る。
		/// </summary>
		// a_resourceManager : 変換元の実体を引く先。保存先のパス解決とサブアセットのGUID発行は、この中のアセットデータベースで行う
		static bool ConvertModelDataToBinary(ResourceManager& a_resourceManager, const std::string& a_filePath);		// ファイルパスから
		static bool ConvertModelDataToBinary(ResourceManager& a_resourceManager, const Engine::GUID& a_guid);			// guidから
		static bool ConvertModelDataToBinary(ResourceManager& a_resourceManager, const ResourceRef<Model>& a_modelHandle);	// ハンドルから

	private:

		// binaryにコンバート
		static void ConvertMaterialToBinary(ResourceManager& a_resourceManager, const std::string& a_basePath,ModelAssetData& a_asset,const ModelRuntimeData& a_runtime);
		static void ConvertMeshToBinary(ResourceManager& a_resourceManager, const std::string& a_basePath,ModelAssetData& a_asset,const ModelRuntimeData& a_runtime);
		static void ConvertAnimationToBinary(ResourceManager& a_resourceManager, const std::string& a_basePath,ModelAssetData& a_asset,const ModelRuntimeData& a_runtime);


		static void ConvertTexture(ResourceManager& a_resourceManager, const ResourceRef<Texture>& a_ref);
		static Texture* GetTexture(ResourceManager& a_resourceManager, const ResourceRef<Texture>& a_ref);

	};
}
