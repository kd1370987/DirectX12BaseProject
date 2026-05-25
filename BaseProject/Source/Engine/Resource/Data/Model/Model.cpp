#include "Model.h"

#include "Engine/Resource/Loader/Model/Importer/ModelImporter.h"
#include "Engine/Resource/Loader/Model/Importer/tinyGLTF/tinyGLTF.h"


#include "../../Manager/ResourceManager/ResourceManager.h"
#include "../../Manager/AssetDatabase/AssetDatabase.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

namespace Engine::Resource
{
	void Model::Import(const std::string& a_filePath)
	{
		ModelData _model = {};

		// 拡張子とファイルディレクトリ取得
		std::string _ext = FileUtility::GetFilePathExtension(a_filePath);
		std::string _dir = FileUtility::GetDirFromPath(a_filePath);

		// 独自形式があるのかチェック
		auto _originExt = FileUtility::FindExtensionInDirectory(_dir, "ojmdl");
		//-------------------------------------
		// 独自形式読み込み
		//-------------------------------------
		if (_originExt.size() > 0)
		{
			//Load(_dir);
		}
		//-------------------------------------
		// TinyGLTFを使用
		//-------------------------------------
		//else if (_ext == "gltf")
		if (_ext == "gltf")
		{
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
			for (auto& _node : m_originalNodes)
			{
				_node.nodeNameHash = StringUtility::ToHash(_node.name);
			}

			m_rootNodeIndices = std::move(_model.rootNodeIndices);
			m_boneNodeIndices = std::move(_model.boneNodeIndices);
			m_meshNodeIndices = std::move(_model.meshNodeIndices);
			m_collisionMeshNodeIndices = std::move(_model.collisionMeshNodeIndices);
			m_drawMeshNodeIndices = std::move(_model.drawMeshNodeIndices);

			m_name = FileUtility::GetFileName(a_filePath);
		}
		//-------------------------------------
		// Assimpを使用
		//-------------------------------------
		else
		{
			assert(0 && "Assimpは未対応");
		}

		// 描画時コマンド用に事前キャッシュを作っておく

		// 描画用meshを持っているノード
		for (auto& _nodeIdx : m_drawMeshNodeIndices)
		{
			for (auto& _meshIdx : m_originalNodes[_nodeIdx].meshIndices)
			{
				// 描画メッシュハンドル取得
				const auto& _meshHandle = m_meshHandleVec[_meshIdx];
				const auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandle);
				// サブセットごとに描画するアイテムを集める
				for (UINT _subIdx = 0; _subIdx < _pMesh->GetMetaData().subsets.size(); ++_subIdx)
				{
					// 面が一枚もなければスキップ
					if (_pMesh->GetMetaData().subsets[_subIdx].faceCount == 0) continue;

					// マテリアルハンドル取得
					const auto& _materialHandle =
						m_materialHandleVec[_pMesh->GetMetaData().subsets[_subIdx].materialNumber];
					const auto* _pMate = Engine::Resource::ResourceManager::Instance().Get(_materialHandle);

					// コマンド作成
					ModelDrawCommand _cmd = {};
					_cmd.nodeIndex = static_cast<uint16_t>(_nodeIdx);
					_cmd.meshRawID = static_cast<uint16_t>(_meshHandle.idx);
					_cmd.materialRawID = static_cast<uint16_t>(_materialHandle.idx);
					_cmd.subIdx = _subIdx;
					_cmd.alphaMode = _pMate->alphaMode;
					m_cachedDrawCommands.push_back(_cmd);
				}

			}
		}
	}
	void Model::Load(const std::string& a_fileDir)
	{
		Persistence::Archive _ar(Persistence::Archive::Mode::Load,m_name, a_fileDir, "mdl");	
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
			std::string basePath ="Asset/Material/" + m_name + std::to_string(_i);
			auto _guid = AssetDatabase::Instance().AddMetaData(basePath,"mtrl","Material");
			m_materialGUIDVec[_i] = _guid;

			// 保存
			_matrial->Save(basePath + _matrial->name, m_name);
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
			std::string basePath ="Asset/Mesh/" + m_name + std::to_string(_i);
			auto _guid = AssetDatabase::Instance().AddMetaData(basePath, "mesh", "Mesh");
			m_meshGUIDVec[_i] = _guid;

			// 保存
			_mesh->Save(basePath + std::to_string(_i), m_name);
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
			std::string basePath = "Asset/Animation/" + m_name + std::to_string(_i);
			auto _guid = AssetDatabase::Instance().AddMetaData(basePath, "anim", "Animation");
			m_animationGUIDVec[_i] = _guid;

			// 保存
			_anim->Save(basePath + _anim->name, m_name);
		}

		// モデルデータの保存
		{
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, a_fileDir, m_name, "mdl");
			_ar.StringField("ModelName", m_name);

			_ar.GUIDVectorField("MaterialGUID", m_materialGUIDVec);
			_ar.GUIDVectorField("MeshGUID", m_meshGUIDVec);
			_ar.GUIDVectorField("AnimationGUID", m_animationGUIDVec);

			for (auto& _node : m_originalNodes)
			{
				_node.Archive(_ar);
			}

			_ar.VectorField("RootNodeIndices", m_rootNodeIndices);
			_ar.VectorField("BoneNodeIndices", m_boneNodeIndices);
			_ar.VectorField("MeshNodeIndices", m_meshNodeIndices);
			_ar.VectorField("CollisionMeshNodeIndices", m_collisionMeshNodeIndices);
			_ar.VectorField("DrawMeshNodeIndices", m_drawMeshNodeIndices);
		}
	}
}