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

		m_originalNodes = std::move(_model.originalNodes);
		m_rootNodeIndices = std::move(_model.rootNodeIndices);
		m_boneNodeIndices = std::move(_model.boneNodeIndices);
		m_meshNodeIndices = std::move(_model.meshNodeIndices);
		m_collisionMeshNodeIndices = std::move(_model.collisionMeshNodeIndices);
		m_drawMeshNodeIndices = std::move(_model.drawMeshNodeIndices);

		m_name = FileUtility::GetFileName(a_filePath);
	}
	//const AnimationData* Engine::Resource::Model::GetAnimation(uint32_t a_clipID) const
	//{
	//	// アニメーション取得
	//	if (a_clipID < m_upAnimationVec.size())
	//	{
	//		return m_upAnimationVec[a_clipID].get();
	//	}

	//	// 取得失敗
	//	return nullptr;
	//}

	//uint32_t Engine::Resource::Model::GetAnimationClipCount(const std::string& a_animeNmae) const
	//{
	//	// アニメーション取得
	//	for (size_t _i = 0; _i < m_upAnimationVec.size(); ++_i)
	//	{
	//		if (m_upAnimationVec[_i]->name == a_animeNmae)
	//		{
	//			return static_cast<uint32_t>(_i);
	//		}
	//	}

	//	assert(false && "アニメーションが見つかりませんでした");
	//	return 0;
	//}
}