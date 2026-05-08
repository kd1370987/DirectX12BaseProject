#include "Model.h"

#include "Engine/Resource/Loader/Model/Importer/ModelImporter.h"

namespace Engine::Resource
{
	void Model::Import(const std::string& a_filePath)
	{
		// いずれこのモデルはランタイム特化にする予定
		auto _model = ImportModel(a_filePath);
		m_name = std::move(_model.name);
		m_upMaterialVec = std::move(_model.upMaterialVec);
		m_upMeshVec = std::move(_model.upMeshVec);
		m_upAnimationVec = std::move(_model.upAnimationVec);
		m_originalNodes = std::move(_model.originalNodes);
		m_rootNodeIndices = std::move(_model.rootNodeIndices);
		m_boneNodeIndices = std::move(_model.boneNodeIndices);
		m_meshNodeIndices = std::move(_model.meshNodeIndices);
		m_collisionMeshNodeIndices = std::move(_model.collisionMeshNodeIndices);
		m_drawMeshNodeIndices = std::move(_model.drawMeshNodeIndices);

		m_name = FileUtility::GetFileName(a_filePath);
	}
	const AnimationData* Engine::Resource::Model::GetAnimation(uint32_t a_clipID) const
	{
		// アニメーション取得
		if (a_clipID < m_upAnimationVec.size())
		{
			return m_upAnimationVec[a_clipID].get();
		}

		// 取得失敗
		return nullptr;
	}

	uint32_t Engine::Resource::Model::GetAnimationClipCount(const std::string& a_animeNmae) const
	{
		// アニメーション取得
		for (size_t _i = 0; _i < m_upAnimationVec.size(); ++_i)
		{
			if (m_upAnimationVec[_i]->name == a_animeNmae)
			{
				return static_cast<uint32_t>(_i);
			}
		}

		assert(false && "アニメーションが見つかりませんでした");
		return 16;
	}
}