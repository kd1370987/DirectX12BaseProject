#include "Model.h"

#include "Engine/Resource/Loader/Model/Importer/ModelImporter.h"

#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	void Model::Import(const std::string& a_filePath)
	{
		// いずれこのモデルはランタイム特化にする予定
		auto _model = ImportModel(a_filePath);
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
}