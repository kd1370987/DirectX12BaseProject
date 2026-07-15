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
			return Load(a_filePath);
		}
		else if (_ext == ".gltf")
		{
			return GLTFLoad(a_filePath);
		}
		else if (_ext == ".ojmdl")
		{
			return Load(a_filePath);
		}
		else
		{
			ENGINE_ERRLOG(false,"この拡張子のモデルは対応していません : %s", _ext.c_str());
			return _resultModel;
		}
	}
	Model ModelIO::Load(const std::string& a_filePath)
	{
		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _dir, _fileName, "mdl");

		ModelAssetData _assetData = {};

		_ar.StringField("ModelName", _assetData.name);

		_ar.GUIDVectorField("MaterialGUID", _assetData.materialGUIDs);
		_ar.GUIDVectorField("MeshGUID", _assetData.meshGUIDs);
		_ar.GUIDVectorField("AnimationGUID", _assetData.animationGUIDs);

		UINT _nodeCount = 0;
		_ar.Field("NodeCount", _nodeCount);
		_assetData.originalNodes.resize(_nodeCount);
		for (UINT _i = 0; _i < _nodeCount; ++_i)
		{
			_assetData.originalNodes[_i].Archive(_ar, _i);
		}

		_ar.VectorField("RootNodeIndices", _assetData.rootNodeIndices);
		_ar.VectorField("BoneNodeIndices", _assetData.boneNodeIndices);
		_ar.VectorField("MeshNodeIndices", _assetData.meshNodeIndices);
		_ar.VectorField("CollisionMeshNodeIndices", _assetData.collisionMeshNodeIndices);
		_ar.VectorField("DrawMeshNodeIndices", _assetData.drawMeshNodeIndices);

		ModelRuntimeData _runtimeData = {};

		// ---- 参照しているデータの復元 ----
		for (const auto& _guid : _assetData.materialGUIDs)
		{
			auto _handle = ResourceManager::Instance().Load<Material>(_guid);
			_runtimeData.materials.push_back(std::move(_handle));
		}
		for (const auto& _guid : _assetData.meshGUIDs)
		{
			auto _handle = ResourceManager::Instance().Load<Mesh>(_guid);
			_runtimeData.meshes.push_back(std::move(_handle));
		}
		for (const auto& _guid : _assetData.animationGUIDs)
		{
			auto _handle = ResourceManager::Instance().Load<AnimationData>(_guid);
			_runtimeData.animations.push_back(std::move(_handle));
		}

		// 各ノードに名前のハッシュ値をつける
		for (auto& _node : _assetData.originalNodes)
		{
			_node.nodeNameHash = StringUtility::ToHash(_node.name);
		}

		// 描画用コマンドの構築
		CreateDrawCmd(_assetData, _runtimeData);

		return Model(std::move(_assetData), std::move(_runtimeData));
	}
	Model ModelIO::GLTFLoad(const std::string& a_filePath)
	{
		// モデルデータのパース
		auto _spRawModel = GLTF::Load(a_filePath);

		// エンジン側の仕様に合わせてコンバート
		GLTF::ModelData _rawModel = *_spRawModel.get();
		auto _model = Converter::ModelConverter::ConvertModelData(a_filePath, _rawModel);

		// エンジン側に登録してランタイムデータとして保存
		ModelAssetData _assetData = {};
		_assetData.name = FileUtility::GetFileName(a_filePath);
		_assetData.originalNodes = std::move(_model.originalNodes);
		_assetData.rootNodeIndices = _model.rootNodeIndices;
		_assetData.boneNodeIndices = _model.boneNodeIndices;
		_assetData.meshNodeIndices = _model.meshNodeIndices;
		_assetData.collisionMeshNodeIndices = _model.collisionMeshNodeIndices;
		_assetData.drawMeshNodeIndices = _model.drawMeshNodeIndices;

		ModelRuntimeData _runtimeData = {};
		for (auto& _material : _model.MaterialVec)
		{
			auto _handle = Resource::ResourceManager::Instance().Add(std::move(_material));
			_runtimeData.materials.push_back(_handle);
		}
		for (auto& _mesh : _model.MeshVec)
		{
			auto _handle = ResourceManager::Instance().Add(std::move(_mesh));
			_runtimeData.meshes.push_back(_handle);
		}
		for (auto& _ani : _model.AnimationVec)
		{
			auto _handle = ResourceManager::Instance().Add(std::move(_ani));
			_runtimeData.animations.push_back(_handle);
		}

		// 各ノードに名前のハッシュ値をつける
		for (auto& _node : _assetData.originalNodes)
		{
			_node.nodeNameHash = StringUtility::ToHash(_node.name);
		}

		// 描画用コマンドの構築
		CreateDrawCmd(_assetData,_runtimeData);

		return Model(std::move(_assetData), std::move(_runtimeData));
	}

	void ModelIO::CreateDrawCmd(const ModelAssetData& a_modelAssetData, ModelRuntimeData& a_runtimeData)
	{
		// 描画時用に事前コマンド構築
		for (auto& _meshNodeIdx : a_modelAssetData.drawMeshNodeIndices)
		{
			// 描画用ノードからメッシュのインデックスを取得
			for (auto& _meshIdx : a_modelAssetData.originalNodes[_meshNodeIdx].meshIndices)
			{
				// 描画メッシュハンドルを取得
				const auto& _meshHandle = a_runtimeData.meshes[_meshIdx];
				auto* _pMesh = ResourceManager::Instance().Ref(_meshHandle);
				if (!_pMesh)
				{
					ENGINE_ERRLOG(false, "メッシュが読み込まれていません");
					continue;
				}

				// サブセットごとに描画アイテムを集める
				for (UINT _subIdx = 0; _subIdx < _pMesh->GetMetaData().subsets.size(); ++_subIdx)
				{
					// 面が一枚もなければスキップ
					if (_pMesh->GetMetaData().subsets[_subIdx].faceCount == 0) continue;

					// マテリアルハンドル取得
					const auto& _materialHandle = a_runtimeData.materials[_pMesh->GetMetaData().subsets[_subIdx].materialNumber];
					auto* _pMate = Engine::Resource::ResourceManager::Instance().Ref(_materialHandle);
					if (!_pMate)
					{
						ENGINE_ERRLOG(false, "マテリアルが読み込まれていません");
						continue;
					}

					// コマンド作成
					ModelDrawCommand _cmd = {};
					_cmd.pMaterial = _pMate;
					_cmd.pMesh = _pMesh;
					_cmd.nodeIndex = static_cast<uint16_t>(_meshNodeIdx);
					_cmd.meshRawID = static_cast<uint16_t>(_meshHandle.GetIndex());
					_cmd.materialRawID = static_cast<uint16_t>(_materialHandle.GetIndex());
					_cmd.subIdx = _subIdx;
					_cmd.alphaMode = _pMate->alphaMode;
					a_runtimeData.drawCommands.push_back(_cmd);
				}
			}
		}
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