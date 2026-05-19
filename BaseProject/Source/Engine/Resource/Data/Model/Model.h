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
		std::vector<Material>		MaterialVec;		// マテリアル
		std::vector <Mesh> 			MeshVec;			// メッシュ
		std::vector <AnimationData>	AnimationVec;		// アニメーション

		// ノード
		std::vector<Node>							originalNodes;			// 全ノード配列
		std::vector<int>							rootNodeIndices;			// Rootノード
		std::vector<int>							boneNodeIndices;			// ボーンノード
		std::vector<int>							meshNodeIndices;			// メッシュが存在するノード
		std::vector<int>							collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
		std::vector<int>							drawMeshNodeIndices;		// 描画するノード


		ModelData() = default;

		ModelData(const ModelData&)
		{
			assert(false && "ModelData copy ctor");
		}

		ModelData& operator=(const ModelData&)
		{
			assert(false && "ModelData copy assign");
			return *this;
		}

		ModelData(ModelData&&) noexcept = default;
		ModelData& operator=(ModelData&&) noexcept = default;
	};

	// モデルデータ
	class Model
	{
	public:

		Model() = default;
		~Model() {};

		// モデル生成
		void Import(const std::string& a_filePath);

		// ---- アクセサ ----
		//const AnimationData* GetAnimation(uint32_t a_clipID) const;					// アニメーション取得
		//uint32_t GetAnimationClipCount(const std::string& a_animeNmae) const;		// アニメーションクリップ取得

		// データのハンドル
		const std::vector<Handle<Material>>&		GetMaterialHandles()	const { return m_materialHandleVec; }
		const std::vector<Handle<Mesh>>&			GetMeshHandles()		const { return m_meshHandleVec; }
		const std::vector<Handle<AnimationData>>&	GetAnimationHandles()	const { return m_animationHandleVec; }

		// モデルが管理する実データ
		const std::string& GetName() const { return m_name; }
		const std::vector<Node>& GetOriginalNodeVec() const { return m_originalNodes; }

		// 各種インデックス配列
		const std::vector<int>& GetRootNodeVec() const { return m_rootNodeIndices; }
		const std::vector<int>& GetBoneNodeVec() const { return m_boneNodeIndices; }
		const std::vector<int>& GetMeshNodeVec() const { return m_meshNodeIndices; }
		const std::vector<int>& GetCollisionMeshNodeVec() const { return m_collisionMeshNodeIndices; }
		const std::vector<int>& GetDrawNodeVec() const { return m_drawMeshNodeIndices; }

	private:

		// モデル名
		std::string m_name;

		// 構成物
		std::vector<Handle<Material>>		m_materialHandleVec = {};
		std::vector<Handle<Mesh>>			m_meshHandleVec = {};
		std::vector<Handle<AnimationData>>	m_animationHandleVec = {};

		// 全ノード情報
		std::vector<Node>							m_originalNodes;

		// ノード
		std::vector<int>							m_rootNodeIndices;			// Rootノード
		std::vector<int>							m_boneNodeIndices;			// ボーンノード
		std::vector<int>							m_meshNodeIndices;			// メッシュが存在するノード
		std::vector<int>							m_collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
		std::vector<int>							m_drawMeshNodeIndices;		// 描画するノード

		
	public:
		
		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		Model(Model&&) noexcept = default;
		Model& operator=(Model&&) noexcept = default;
	};
}
