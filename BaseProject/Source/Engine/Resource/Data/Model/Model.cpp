#include "Model.h"

#include "Engine/Resource/Loader/Model/Importer/ModelImporter.h"
#include "Engine/Resource/Loader/Model/Importer/tinyGLTF/tinyGLTF.h"


#include "../../Manager/ResourceManager/ResourceManager.h"
#include "../../Manager/AssetDatabase/AssetDatabase.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

namespace Engine::Resource
{

	// モデルの構築
	void Model::Import(const std::string& a_filePath)
	{
		// アセットデータベースからメタファイルを検索
		auto* _pAssetProp = AssetDatabase::Instance().GetAssetProperty(a_filePath);
		if (!_pAssetProp)
		{
			ENGINE_LOG("メタファイルが見つからなかったためモデルの読み込みに失敗");
			return;
		}	

		// 優先度の高い拡張子のタイプを検索
		auto _ext = FinddExtension(_pAssetProp->extensionsVec);

		if (_ext == ".obmdl")
		{
			Load(a_filePath);
		}
		else if (_ext == ".gltf")
		{
			// パース時の出力用
			ModelData _model = {};
			_model = GLTF::Import(a_filePath);
			m_name = std::move(_model.name);

			for (auto& _mate : _model.MaterialVec)
			{
				auto _handle = ResourceManager::Instance().Add(std::move(_mate));
				m_materialHandleVec.push_back(_handle);
			}
			for (auto& _mesh : _model.MeshVec)
			{
				auto _handle = ResourceManager::Instance().Add(std::move(_mesh));
				m_meshHandleVec.push_back(_handle);
			}
			for (auto& _ani : _model.AnimationVec)
			{
				auto _handle = ResourceManager::Instance().Add(std::move(_ani));
				m_animationHandleVec.push_back(_handle);
			}

			// ノードすべてに名前のハッシュ値をつける
			m_originalNodes = std::move(_model.originalNodes);

			m_rootNodeIndices = std::move(_model.rootNodeIndices);
			m_boneNodeIndices = std::move(_model.boneNodeIndices);
			m_meshNodeIndices = std::move(_model.meshNodeIndices);
			m_collisionMeshNodeIndices = std::move(_model.collisionMeshNodeIndices);
			m_drawMeshNodeIndices = std::move(_model.drawMeshNodeIndices);

			m_name = FileUtility::GetFileName(a_filePath);
		}
		else if (_ext == ".ojmdl")
		{
			Load(a_filePath);
		}
		else
		{
			ENGINE_LOG("この拡張子のモデルは対応していません : %s",_ext.c_str());
			return;
		}
	
		// 描画用事前コマンド構築
		BuildDrawCmdCach();
	}
	void Model::Load(const std::string& a_fileDir)
	{
		auto _dir = FileUtility::GetDirFromPath(a_fileDir);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_fileDir);
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _dir, _fileName, "mdl");
		_ar.StringField("ModelName", m_name);

		_ar.GUIDVectorField("MaterialGUID", m_materialGUIDVec);
		_ar.GUIDVectorField("MeshGUID", m_meshGUIDVec);
		_ar.GUIDVectorField("AnimationGUID", m_animationGUIDVec);

		UINT _nodeCount = 0;
		_ar.Field("NodeCount", _nodeCount);
		m_originalNodes.resize(_nodeCount);
		for (UINT _i = 0; _i < _nodeCount; ++_i)
		{
			m_originalNodes[_i].Archive(_ar, _i);
		}

		_ar.VectorField("RootNodeIndices", m_rootNodeIndices);
		_ar.VectorField("BoneNodeIndices", m_boneNodeIndices);
		_ar.VectorField("MeshNodeIndices", m_meshNodeIndices);
		_ar.VectorField("CollisionMeshNodeIndices", m_collisionMeshNodeIndices);
		_ar.VectorField("DrawMeshNodeIndices", m_drawMeshNodeIndices);

		// ---- 参照しているデータの復元 ----
		for (const auto& _guid : m_materialGUIDVec)
		{
			auto _handle = ResourceManager::Instance().Load<Material>(_guid);
			m_materialHandleVec.push_back(std::move(_handle));
		}
		for (const auto& _guid : m_meshGUIDVec)
		{
			auto _handle = ResourceManager::Instance().Load<Mesh>(_guid);
			m_meshHandleVec.push_back(std::move(_handle));
		}
		for (const auto& _guid : m_animationGUIDVec)
		{
			auto _handle = ResourceManager::Instance().Load<AnimationData>(_guid);
			m_animationHandleVec.push_back(std::move(_handle));
		}
	}

	void Model::Save(const std::string& a_fileDir)
	{
		// ---- 参照しているデータもセーブ ----
		// マテリアルの保存
		UINT _mtrlHandleSize = m_materialHandleVec.size();
		m_materialGUIDVec.resize(_mtrlHandleSize);
		for(UINT _i = 0; _i < _mtrlHandleSize; ++_i)
		{
			// マテリアル取得
			auto _mateHandle = m_materialHandleVec[_i];
			auto* _matrial = Resource::ResourceManager::Instance().Ref(_mateHandle);

			// 保存データ作成
			auto _fileName = FileUtility::GetFileNameWithoutExtension(m_name) + std::to_string(_i);
			std::string basePath ="Asset/Material/" + _fileName;

			// 保存
			auto _fullBasePath = basePath + "/" + _fileName;													// 拡張子なしのパス
			m_materialGUIDVec[_i] = AssetDatabase::Instance().AddMetaData(_fullBasePath, "Material");	// メタファイルを作成
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, basePath, _fileName, "mtrl");
			_matrial->Archive(_ar);
			//_matrial->Save(basePath, _fileName);														// メタファイルの隣にデータ作成

		}
		// メッシュの保存
		UINT _meshHandleSize = m_meshHandleVec.size();
		m_meshGUIDVec.resize(_meshHandleSize);
		for (UINT _i = 0; _i < _meshHandleSize; ++_i)
		{
			// メッシュ取得
			auto _meshHandle = m_meshHandleVec[_i];
			auto* _mesh = Resource::ResourceManager::Instance().Ref(_meshHandle);

			// 保存データ作成
			auto _fileName = FileUtility::GetFileNameWithoutExtension(m_name) + std::to_string(_i);
			std::string basePath ="Asset/Mesh/" + _fileName;

			// 保存
			auto _fullBasePath = basePath + "/" + _fileName;
			m_meshGUIDVec[_i] = AssetDatabase::Instance().AddMetaData(_fullBasePath, "Mesh");
			_mesh->Save(basePath, _fileName);

		}
		// アニメーションの保存
		UINT _animHandleSize = m_animationHandleVec.size();
		m_animationGUIDVec.resize(_animHandleSize);
		for (UINT _i = 0; _i < _animHandleSize; ++_i)
		{
			// アニメーションの取得
			auto _animHandle = m_animationHandleVec[_i];
			auto* _anim = Resource::ResourceManager::Instance().Ref(_animHandle);

			// 保存データ作成
			auto _fileName = FileUtility::GetFileNameWithoutExtension(m_name) + std::to_string(_i);
			std::string basePath = "Asset/Animation/" + _fileName;

			// 保存
			auto _fullBasePath = basePath + "/" + _fileName;
			m_animationGUIDVec[_i] = AssetDatabase::Instance().AddMetaData(_fullBasePath, "Animation");
			_anim->Save(basePath, _fileName);
		}

		// モデルデータの保存
		auto _dir = FileUtility::GetDirFromPath(a_fileDir);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_fileDir);
		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _dir, _fileName, "mdl");
		_ar.StringField("ModelName", m_name);

		_ar.GUIDVectorField("MaterialGUID", m_materialGUIDVec);
		_ar.GUIDVectorField("MeshGUID", m_meshGUIDVec);
		_ar.GUIDVectorField("AnimationGUID", m_animationGUIDVec);

		UINT _nodeCount = m_originalNodes.size();
		_ar.Field("NodeCount",_nodeCount);
		for (UINT _i = 0; _i < _nodeCount; ++_i)
		{
			m_originalNodes[_i].Archive(_ar, _i);
		}

		_ar.VectorField("RootNodeIndices", m_rootNodeIndices);
		_ar.VectorField("BoneNodeIndices", m_boneNodeIndices);
		_ar.VectorField("MeshNodeIndices", m_meshNodeIndices);
		_ar.VectorField("CollisionMeshNodeIndices", m_collisionMeshNodeIndices);
		_ar.VectorField("DrawMeshNodeIndices", m_drawMeshNodeIndices);
		
	}
	void Model::Release()
	{
		m_name.clear();
		m_materialGUIDVec.clear();
		m_meshGUIDVec.clear();
		m_animationGUIDVec.clear();

		m_originalNodes.clear();

		// ノード
		m_rootNodeIndices.clear();			// Rootノード
		m_boneNodeIndices.clear();			// ボーンノード
		m_meshNodeIndices.clear();			// メッシュが存在するノード
		m_collisionMeshNodeIndices.clear();	// 子リジョンメッシュが存在するノード
		m_drawMeshNodeIndices.clear();		// 描画するノード

		// ---- ランタイムデータ ----
		m_materialHandleVec.clear();
		m_meshHandleVec.clear();
		m_animationHandleVec.clear();

		// 描画コマンド用事前キャッシュ
		m_cachedDrawCommands.clear();
	}
	Engine::GUID Model::GetAnimationGUIDFromHandle(const Handle<AnimationData>& a_handle) const
	{
		// セーブ時の1回だけ線形探索
		for (size_t _i = 0; _i < m_animationHandleVec.size(); ++_i)
		{
			if (m_animationHandleVec[_i] == a_handle)
			{
				// 同じインデックスのGUIDを返す
				return m_animationGUIDVec[_i];
			}
		}

		// 見つからなかった場合
		return Engine::GUID();
	}
	Handle<AnimationData> Model::GetAnimationHandleFromGUID(const Engine::GUID& a_guid) const
	{
		// セーブ時の1回だけ線形探索
		for (size_t _i = 0; _i < m_animationGUIDVec.size(); ++_i)
		{
			if (m_animationGUIDVec[_i] == a_guid)
			{
				// 同じインデックスのGUIDを返す
				return m_animationHandleVec[_i];
			}
		}

		// 見つからなかった場合
		return Handle<AnimationData>();
	}
	void Model::BuildDrawCmdCach()
	{
		// ノードに名前のハッシュを作る
		for (auto& _node : m_originalNodes)
		{
			_node.nodeNameHash = StringUtility::ToHash(_node.name);
		}

		// 描画時コマンド用に事前キャッシュを作っておく
		// 描画用meshを持っているノード
		for (auto& _nodeIdx : m_drawMeshNodeIndices)
		{
			for (auto& _meshIdx : m_originalNodes[_nodeIdx].meshIndices)
			{
				// 描画メッシュハンドル取得
				const auto& _meshHandle = m_meshHandleVec[_meshIdx];
				auto* _pMesh = Engine::Resource::ResourceManager::Instance().Ref(_meshHandle);
				// サブセットごとに描画するアイテムを集める
				for (UINT _subIdx = 0; _subIdx < _pMesh->GetMetaData().subsets.size(); ++_subIdx)
				{
					// 面が一枚もなければスキップ
					if (_pMesh->GetMetaData().subsets[_subIdx].faceCount == 0) continue;

					// マテリアルハンドル取得
					const auto& _materialHandle =
						m_materialHandleVec[_pMesh->GetMetaData().subsets[_subIdx].materialNumber];
					auto* _pMate = Engine::Resource::ResourceManager::Instance().Ref(_materialHandle);

					// コマンド作成
					ModelDrawCommand _cmd = {};
					_cmd.pMaterial = _pMate;
					_cmd.pMesh = _pMesh;
					_cmd.nodeIndex = static_cast<uint16_t>(_nodeIdx);
					_cmd.meshRawID = static_cast<uint16_t>(_meshHandle.GetIndex());
					_cmd.materialRawID = static_cast<uint16_t>(_materialHandle.GetIndex());
					_cmd.subIdx = _subIdx;
					_cmd.alphaMode = _pMate->alphaMode;
					m_cachedDrawCommands.push_back(_cmd);
				}

			}
		}
	}
	std::string Model::FinddExtension(const std::vector<std::string>& a_extVed)
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
				if(_tier > EExtTier::Default)
				{
					_res = ".gltf";
					_tier = EExtTier::Default;
				}
			}
			else if (a_extVed[_i] == ".ojmdl")
			{
				if(_tier > EExtTier::OJ)
				{
					_res = ".ojmdl";
					_tier = EExtTier::OJ;
				}
			}
		}

		return _res;
	}
}