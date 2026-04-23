#pragma once

#include "../Material/Material.h"
#include "../Animation/Animation.h"
#include "../Mesh/Mesh.h"
#include "../Node/Node.h"

namespace Engine::Resource
{
	struct ModelData
	{
		// モデル名
		std::string name;

		// マテリアル
		std::vector<Material>						materials;

		// メッシュの配列
		std::vector<std::shared_ptr<Mesh>> 			spMeshVec;

		// アニメーション
		std::vector<std::shared_ptr<AnimationData>> spAnimations;

		// ノード
		std::vector<Node>							originalNodes;			// 全ノード配列
		std::vector<int>							rootNodeIndices;			// Rootノード
		std::vector<int>							boneNodeIndices;			// ボーンノード
		std::vector<int>							meshNodeIndices;			// メッシュが存在するノード
		std::vector<int>							collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
		std::vector<int>							drawMeshNodeIndices;		// 描画するノード
	};

	// モデルデータ
	class Model
	{
	public:

		Model() = default;
		//~Model() {};

		// モデル生成
		void Import(const std::string& a_filePath);

		// アクセサ
		std::shared_ptr<AnimationData> GetSPAnimation(uint32_t a_clipID) const;		// アニメーション取得
		uint32_t GetAnimationClipCount(const std::string& a_animeNmae) const;		// アニメーションクリップ取得

		const std::string& GetName() const { return m_name; }
		const std::vector<Material>& GetMaterialVec() const { return m_materials; }
		const std::vector<std::shared_ptr<Mesh>>& GetSPMeshVec() const { return m_spMeshVec; }
		const std::vector<std::shared_ptr<AnimationData>>& GetSPAnimationVec()const { return m_spAnimations; }
		const std::vector<Node>& GetOriginalNodeVec() const { return m_originalNodes; }
		const std::vector<int>& GetRootNodeVec() const { return m_rootNodeIndices; }
		const std::vector<int>& GetBoneNodeVec() const { return m_boneNodeIndices; }
		const std::vector<int>& GetMeshNodeVec() const { return m_meshNodeIndices; }
		const std::vector<int>& GetCollisionMeshNodeVec() const { return m_collisionMeshNodeIndices; }
		const std::vector<int>& GetDrawNodeVec() const { return m_drawMeshNodeIndices; }

	private:

		// モデル名
		std::string m_name;

		// マテリアル
		std::vector<Material>						m_materials;

		// メッシュの配列
		std::vector<std::shared_ptr<Mesh>> 			m_spMeshVec;

		// アニメーション
		std::vector<std::shared_ptr<AnimationData>> m_spAnimations;

		// ノード
		std::vector<Node>							m_originalNodes;			// 全ノード配列
		std::vector<int>							m_rootNodeIndices;			// Rootノード
		std::vector<int>							m_boneNodeIndices;			// ボーンノード
		std::vector<int>							m_meshNodeIndices;			// メッシュが存在するノード
		std::vector<int>							m_collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
		std::vector<int>							m_drawMeshNodeIndices;		// 描画するノード
	};
}
