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
	};

	// モデル描画時用キャッシュデータ
	struct ModelDrawCommand
	{
		Mesh* pMesh = nullptr;
		Material* pMaterial = nullptr;
		uint16_t nodeIndex;      // モデル内のローカルノード番号
		uint16_t meshRawID;      // ResourceManager 内の配列インデックス(Raw ID)
		uint16_t materialRawID;  // ResourceManager 内の配列インデックス(Raw ID)
		uint8_t  subIdx;
		Engine::Resource::Alpha alphaMode;
	};
	struct ModelAssetData
	{
		std::string name;

		std::vector<GUID> materialGUIDs;
		std::vector<GUID> meshGUIDs;
		std::vector<GUID> animationGUIDs;

		std::vector<Node> nodes;

		std::vector<int> rootNodes;
		std::vector<int> boneNodes;
		std::vector<int> meshNodes;
		std::vector<int> collisionNodes;
		std::vector<int> drawNodes;
	};

	struct ModelRuntimeData
	{
		std::vector<ResourceRef<Material>> materials;
		std::vector<ResourceRef<Mesh>> meshes;
		std::vector<ResourceRef<AnimationData>> animations;

		std::vector<ModelDrawCommand> drawCommands;
	};
	// モデルデータ
	class Model
	{
	public:

		Model() = default;
		~Model() = default;
		NON_COPYABLE_MOVABLE(Model);


		// モデル生成
		void Import(const std::string& a_filePath);
		void Load(const std::string& a_filePath);
		void Save(const std::string& a_filePath);

		// 解放
		void Release();

		// ---- アクセサ ----
		// データのハンドル
		const std::vector<ResourceRef<Material>>&		GetMaterialHandles()	const { return m_materialHandleVec; }
		const std::vector<ResourceRef<Mesh>>&			GetMeshHandles()		const { return m_meshHandleVec; }
		const std::vector<ResourceRef<AnimationData>>&	GetAnimationHandles()	const { return m_animationHandleVec; }

		// アニメーションのハンドルからGUIDを逆引き
		Engine::GUID GetAnimationGUIDFromHandle(const Handle<AnimationData>& a_handle) const;
		Handle<AnimationData> GetAnimationHandleFromGUID(const Engine::GUID& a_guid) const;

		// モデルが管理する実データ
		const std::string& GetName() const { return m_name; }
		const std::vector<Node>& GetOriginalNodeVec() const { return m_originalNodes; }

		// 各種インデックス配列
		const std::vector<int>& GetRootNodeVec() const { return m_rootNodeIndices; }
		const std::vector<int>& GetBoneNodeVec() const { return m_boneNodeIndices; }
		const std::vector<int>& GetMeshNodeVec() const { return m_meshNodeIndices; }
		const std::vector<int>& GetCollisionMeshNodeVec() const { return m_collisionMeshNodeIndices; }
		const std::vector<int>& GetDrawNodeVec() const { return m_drawMeshNodeIndices; }

		// 描画時用コマンド取得
		const std::vector<ModelDrawCommand>& GetDrawCommandVec() const { return m_cachedDrawCommands; }
	private:

		// 描画用のコマンドを事前構築
		void BuildDrawCmdCach();


		// 優先順位の高い拡張子を検索
		std::string FinddExtension(const std::vector<std::string>& a_extVed);

	private:

		// ---- シリアライズ用データ ----
		// モデル名
		std::string m_name;

		// 保持データ
		std::vector<Engine::GUID> m_materialGUIDVec = {};
		std::vector<Engine::GUID> m_meshGUIDVec = {};
		std::vector<Engine::GUID> m_animationGUIDVec = {};

		// 全ノード情報
		std::vector<Node>							m_originalNodes;

		// ノード
		std::vector<int>							m_rootNodeIndices;			// Rootノード
		std::vector<int>							m_boneNodeIndices;			// ボーンノード
		std::vector<int>							m_meshNodeIndices;			// メッシュが存在するノード
		std::vector<int>							m_collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
		std::vector<int>							m_drawMeshNodeIndices;		// 描画するノード

		// ---- ランタイムデータ ----
		std::vector<ResourceRef<Material>>		m_materialHandleVec = {};
		std::vector<ResourceRef<Mesh>>			m_meshHandleVec = {};
		std::vector<ResourceRef<AnimationData>>	m_animationHandleVec = {};

		// 描画コマンド用事前キャッシュ
		std::vector<ModelDrawCommand> m_cachedDrawCommands;

		ModelAssetData m_AssetData = {};
		ModelRuntimeData m_runtimeData = {};
	};
}
