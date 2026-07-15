#include "Model.h"

#include "Engine/Resource/Loader/Model/Importer/ModelImporter.h"
#include "Engine/Resource/Loader/Model/Importer/tinyGLTF/tinyGLTF.h"


#include "../../Manager/ResourceManager/ResourceManager.h"
#include "../../Manager/AssetDatabase/AssetDatabase.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

namespace Engine::Resource
{
	void Model::Save(const std::string& a_fileDir)
	{
		// ---- 参照しているデータもセーブ ----
		// マテリアルの保存
		UINT _mtrlHandleSize = m_runtimeData.materials.size();
		m_AssetData.materialGUIDs.resize(_mtrlHandleSize);
		for(UINT _i = 0; _i < _mtrlHandleSize; ++_i)
		{
			// マテリアル取得
			auto _mateHandle = m_runtimeData.materials[_i];
			auto* _matrial = Resource::ResourceManager::Instance().Ref(_mateHandle);

			// 保存データ作成
			auto _fileName = FileUtility::GetFileNameWithoutExtension(m_AssetData.name) + std::to_string(_i);
			std::string basePath ="Asset/Material/" + _fileName;

			// 保存
			auto _fullBasePath = basePath + "/" + _fileName;													// 拡張子なしのパス
			m_AssetData.materialGUIDs[_i] = AssetDatabase::Instance().AddMetaData(_fullBasePath, "Material");	// メタファイルを作成
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, basePath, _fileName, "mtrl");
			_matrial->Archive(_ar);
			//_matrial->Save(basePath, _fileName);														// メタファイルの隣にデータ作成

		}
		// メッシュの保存
		UINT _meshHandleSize = m_runtimeData.meshes.size();
		m_AssetData.meshGUIDs.resize(_meshHandleSize);
		for (UINT _i = 0; _i < _meshHandleSize; ++_i)
		{
			// メッシュ取得
			auto _meshHandle = m_runtimeData.meshes[_i];
			auto* _mesh = Resource::ResourceManager::Instance().Ref(_meshHandle);

			// 保存データ作成
			auto _fileName = FileUtility::GetFileNameWithoutExtension(m_AssetData.name) + std::to_string(_i);
			std::string basePath ="Asset/Mesh/" + _fileName;

			// 保存
			auto _fullBasePath = basePath + "/" + _fileName;
			m_AssetData.meshGUIDs[_i] = AssetDatabase::Instance().AddMetaData(_fullBasePath, "Mesh");
			_mesh->Save(basePath, _fileName);

		}
		// アニメーションの保存
		UINT _animHandleSize = m_runtimeData.animations.size();
		m_AssetData.animationGUIDs.resize(_animHandleSize);
		for (UINT _i = 0; _i < _animHandleSize; ++_i)
		{
			// アニメーションの取得
			auto _animHandle = m_runtimeData.animations[_i];
			auto* _anim = Resource::ResourceManager::Instance().Ref(_animHandle);

			// 保存データ作成
			auto _fileName = FileUtility::GetFileNameWithoutExtension(m_AssetData.name) + std::to_string(_i);
			std::string basePath = "Asset/Animation/" + _fileName;

			// 保存
			auto _fullBasePath = basePath + "/" + _fileName;
			m_AssetData.animationGUIDs[_i] = AssetDatabase::Instance().AddMetaData(_fullBasePath, "Animation");
			_anim->Save(basePath, _fileName);
		}

		// モデルデータの保存
		auto _dir = FileUtility::GetDirFromPath(a_fileDir);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_fileDir);
		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _dir, _fileName, "mdl");
		_ar.StringField("ModelName", m_AssetData.name);

		_ar.GUIDVectorField("MaterialGUID", m_AssetData.materialGUIDs);
		_ar.GUIDVectorField("MeshGUID", m_AssetData.meshGUIDs);
		_ar.GUIDVectorField("AnimationGUID", m_AssetData.animationGUIDs);

		UINT _nodeCount = m_AssetData.originalNodes.size();
		_ar.Field("NodeCount",_nodeCount);
		for (UINT _i = 0; _i < _nodeCount; ++_i)
		{
			m_AssetData.originalNodes[_i].Archive(_ar, _i);
		}

		_ar.VectorField("RootNodeIndices", m_AssetData.rootNodeIndices);
		_ar.VectorField("BoneNodeIndices", m_AssetData.boneNodeIndices);
		_ar.VectorField("MeshNodeIndices", m_AssetData.meshNodeIndices);
		_ar.VectorField("CollisionMeshNodeIndices", m_AssetData.collisionMeshNodeIndices);
		_ar.VectorField("DrawMeshNodeIndices", m_AssetData.drawMeshNodeIndices);
		
	}
	void Model::Release()
	{
		
	}
	Engine::GUID Model::GetAnimationGUIDFromHandle(const Handle<AnimationData>& a_handle) const
	{
		// セーブ時の1回だけ線形探索
		for (size_t _i = 0; _i < m_runtimeData.animations.size(); ++_i)
		{
			if (m_runtimeData.animations[_i] == a_handle)
			{
				// 同じインデックスのGUIDを返す
				return m_AssetData.animationGUIDs[_i];
			}
		}

		// 見つからなかった場合
		return Engine::GUID();
	}
	Handle<AnimationData> Model::GetAnimationHandleFromGUID(const Engine::GUID& a_guid) const
	{
		// セーブ時の1回だけ線形探索
		for (size_t _i = 0; _i < m_AssetData.animationGUIDs.size(); ++_i)
		{
			if (m_AssetData.animationGUIDs[_i] == a_guid)
			{
				// 同じインデックスのGUIDを返す
				return m_runtimeData.animations[_i];
			}
		}

		// 見つからなかった場合
		return Handle<AnimationData>();
	}
}