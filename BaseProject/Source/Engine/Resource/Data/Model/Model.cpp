#include "Model.h"

#include "Engine/Resource/Importer/Model/ModelImporter.h"

namespace Engine::Resource
{
	void Model::Import(const std::string& a_filePath)
	{
		// いずれこのモデルはランタイム特化にする予定
		auto _model = ImportModel(a_filePath);
		m_name = _model.name;
		m_materials = _model.materials;
		m_spMeshVec = _model.spMeshVec;
		m_spAnimations = _model.spAnimations;
		m_originalNodes = _model.originalNodes;
		m_rootNodeIndices = _model.rootNodeIndices;
		m_boneNodeIndices = _model.boneNodeIndices;
		m_meshNodeIndices = _model.meshNodeIndices;
		m_collisionMeshNodeIndices = _model.collisionMeshNodeIndices;
		m_drawMeshNodeIndices = _model.collisionMeshNodeIndices;
	}
	std::shared_ptr<AnimationData> Engine::Resource::Model::GetSPAnimation(uint32_t a_clipID) const
	{
		// アニメーション取得
		std::shared_ptr<Engine::Resource::AnimationData> _spAni = nullptr;

		if (a_clipID < m_spAnimations.size())
		{
			_spAni = m_spAnimations[a_clipID];
		}

		return _spAni;
	}

	uint32_t Engine::Resource::Model::GetAnimationClipCount(const std::string& a_animeNmae) const
	{
		// アニメーション取得
		for (size_t _i = 0; _i < m_spAnimations.size(); ++_i)
		{
			if (m_spAnimations[_i]->name == a_animeNmae)
			{
				return static_cast<uint32_t>(_i);
			}
		}

		assert(false && "アニメーションが見つかりませんでした");
		return 16;
	}
}