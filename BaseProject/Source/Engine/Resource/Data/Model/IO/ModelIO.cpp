#include "ModelIO.h"

#include "ModelConverter/ModelConverter.h"

#include "Parser/tinyGLTF/tinyGLTFParser.h"

#include "../../../Loader/Model/Importer/tinyGLTF/tinyGLTF.h"
namespace Engine::Resource
{
	Model ModelIO::Import(const std::string& a_filePath)
	{
		// アセットデータベースからメタファイルを検索
		auto* _pAssetProp = AssetDatabase::Instance().GetAssetProperty(a_filePath);
		if (!_pAssetProp)
		{
			ENGINE_LOG("メタファイルが見つからなかったためモデルの読み込みに失敗");
			return Model();
		}

		// 優先度の高い拡張子のタイプを検索
		auto _ext = FinddExtension(_pAssetProp->extensionsVec);

		Model _resultModel = {};

		// 拡張子ごとに読み込み方を変更する
		if (_ext == ".obmdl")
		{
			_resultModel.Load(a_filePath);
		}
		else if (_ext == ".gltf")
		{
			auto _spRawModel = GLTF::Load(a_filePath);
			GLTF::ModelData _rawModel = *_spRawModel.get();
			auto _model = Converter::ModelConverter::ConvertModelData(a_filePath, _rawModel);
		}
		else if (_ext == ".ojmdl")
		{
			_resultModel.Load(a_filePath);
		}
		else
		{
			ENGINE_ERRLOG(false,"この拡張子のモデルは対応していません : %s", _ext.c_str());
			return _resultModel;
		}
		auto _rawModel = GLTF::Load(a_filePath);

		return _resultModel;
	}
	void ModelIO::Load(const std::string& a_filePath)
	{
	}
	std::string ModelIO::FinddExtension(const std::vector<std::string>& a_extVed)
	{
		enum class EExtTier
		{
			OB,				// binary ".obmdl",
			Default,		// デフォルトのアセット拡張子".gltf",
			OJ				// JSON  ".ojmdl"
		};

		std::string _res = "";

		EExtTier _tier = EExtTier::OJ;
		for (size_t _i = 0; _i < a_extVed.size(); ++_i)
		{
			if (a_extVed[_i] == ".obmdl")
			{
				_res = ".obmdl";
				break;
			}
			else if (a_extVed[_i] == ".gltf")
			{
				if (_tier > EExtTier::Default)
				{
					_res = ".gltf";
					_tier = EExtTier::Default;
				}
			}
			else if (a_extVed[_i] == ".ojmdl")
			{
				if (_tier > EExtTier::OJ)
				{
					_res = ".ojmdl";
					_tier = EExtTier::OJ;
				}
			}
		}

		return _res;
	}
}