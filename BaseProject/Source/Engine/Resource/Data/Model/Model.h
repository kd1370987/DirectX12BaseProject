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

		std::vector<Node> originalNodes;

		std::vector<int> rootNodeIndices;
		std::vector<int> boneNodeIndices;
		std::vector<int> meshNodeIndices;
		std::vector<int> collisionMeshNodeIndices;
		std::vector<int> drawMeshNodeIndices;
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
		Model(ModelAssetData&& a_asset, ModelRuntimeData&& a_runtimeData) 
			: m_AssetData(std::move(a_asset)), m_runtimeData(std::move(a_runtimeData))
		{}
		~Model() = default;
		NON_COPYABLE_MOVABLE(Model);

		void Save(const std::string& a_filePath);

		// 解放
		void Release();


		// アニメーションのハンドルからGUIDを逆引き
		Engine::GUID GetAnimationGUIDFromHandle(const Handle<AnimationData>& a_handle) const;
		Handle<AnimationData> GetAnimationHandleFromGUID(const Engine::GUID& a_guid) const;

		// ---- アクセサ ----
		// データのハンドル
		const std::vector<ResourceRef<Material>>&		GetMaterialHandles()	const { return m_runtimeData.materials; }
		const std::vector<ResourceRef<Mesh>>&			GetMeshHandles()		const { return m_runtimeData.meshes; }
		const std::vector<ResourceRef<AnimationData>>&	GetAnimationHandles()	const { return m_runtimeData.animations; }

		// モデルが管理する実データ
		const std::string& GetName() const { return m_AssetData.name; }
		const std::vector<Node>& GetOriginalNodeVec() const { return m_AssetData.originalNodes; }

		// 各種インデックス配列
		const std::vector<int>& GetRootNodeVec() const { return m_AssetData.rootNodeIndices; }
		const std::vector<int>& GetBoneNodeVec() const { return m_AssetData.boneNodeIndices; }
		const std::vector<int>& GetMeshNodeVec() const { return m_AssetData.meshNodeIndices; }
		const std::vector<int>& GetCollisionMeshNodeVec() const { return m_AssetData.collisionMeshNodeIndices; }
		const std::vector<int>& GetDrawNodeVec() const { return m_AssetData.drawMeshNodeIndices; }

		// 描画時用コマンド取得
		const std::vector<ModelDrawCommand>& GetDrawCommandVec() const { return m_runtimeData.drawCommands; }

	private:

		// アセットデータ
		ModelAssetData m_AssetData = {};

		// ランタイムデータ
		ModelRuntimeData m_runtimeData = {};
	};
}
