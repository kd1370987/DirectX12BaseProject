#pragma once
namespace Engine::Resource
{
	namespace GLTF
	{
		// GLTFデータをモデルとして読み込み
		Engine::Resource::Model Import(const std::string& a_filePath);

		// GLTFモデルの読み込み
		std::shared_ptr<Engine::Resource::GLTF::ModelData> Load(std::string_view a_filePath);

		// モデルデータに変換
		Engine::Resource::Model Serialize(std::shared_ptr<Engine::Resource::GLTF::ModelData>);

		//=========================================================
		// GLTFマテリアルデータ
		//=========================================================
		struct Material
		{
			//---------------------------------
			// 基本情報
			//---------------------------------
			std::string			name;							// マテリアル名（ないことある）

			// 透明
			std::string			alphaMode = "OPAQUE";			// 透明モード設定
			float				AlphaCutoff = 0.5f;				// 半透明モードのときの閾値
			bool				doubleSided = false;			// 両面するかどうか

			//---------------------------------
			// PBR材質データ
			//---------------------------------

			// 基本色テクスチャ
			std::string			baseColorTexName;				// ファイル名
			DirectX::XMFLOAT4	baseColorFactor = { 1,1,1,1 };	// 乗算用

			// 金属性・粗さ
			std::string			metallicRoughnessTexName;		// 緑成分 = ラフネス 青成分 = メタリック
			float				metallicFactor = 1.0f;			// メタリックの乗算用、テクスチャがなければそのまま使用
			float				roughnessFactor = 1.0f;			// ラフネスの乗算用、テクスチャがなければそのまま使用

			// エミッシブ
			std::string			emissiveTexName;				// エミッシブのテクスチャ名 RGB使用
			DirectX::XMFLOAT3	emissiveFactor = { 1,1,1 };		// エミッシブの乗算用

			//---------------------------------
			// その他のテクスチャ
			//---------------------------------
			// 法線マップ
			std::string			normalTexName;					// テクスチャ名

			// 光の遮蔽度テクスチャ
			std::string			occlusionTexName;				// 赤成分のみ使用
		};

		//=========================================================
		// GLTFノードデータ
		//=========================================================
		struct Node
		{
			//---------------------------------
			// 基本情報
			//---------------------------------
			std::string				name = "none";					// 名前
			std::vector<int>		children = {};				// 子Indexリスト
			int						parent = -1;			// 親のIndex
			int						boneNodeIndex = -1;		// ボーンの場合のIndex

			DirectX::XMFLOAT4X4		localTransform = {};			// ローカル行列	中心点からの座標
			DirectX::XMFLOAT4X4		worldTransform = {};			// ワールド行列 中心点
			DirectX::XMFLOAT4X4		inverseBindMatrix = {};		// ボーンのオフセット行列

			//---------------------------------
			// Mesh専用
			//---------------------------------
			bool						isMesh = false;		// メッシュがあるかどうか
			struct NodeMesh
			{
				std::vector<Engine::Resource::MeshVertex8bit>	vertices = {};			// 頂点配列
				std::vector<Engine::Resource::MeshFace>	faces = {};				// 面情報配列
				std::vector<Engine::Resource::MeshSubset> subsets = {};			// サブセット配列

				bool					isSkinMesh = false;	// スキンメッシュはあるかどうか
			};
			NodeMesh					nodeMesh = {};
		};

		//=========================================================
		// GLTFアニメーションデータ
		//=========================================================

		struct AnimationData
		{
			//---------------------------------
			// 基本情報
			//---------------------------------
			std::string									name = "none";					// 名前
			float										maxLength = 0;			// アニメーションの長さ
			std::vector<std::shared_ptr<Engine::Resource::AnimationNode>> spAnimationNodes = { nullptr };		// 全ノード用アニメーションデータ
		};

		//=========================================================
		// モデルデータ(一つのGLTFに対してついになるもの,中間素材)
		//=========================================================
		struct ModelData
		{
			//---------------------------------
			// ノードデータ
			//---------------------------------
			std::vector<Node>		nodes = {};							// 全ノードデータ
			std::vector<int>			rootNodeIndices = {};				// ルートノードのみのIndexリスト
			std::vector<int>			boneNodeIndices = {};				// ボーンノードのみのIndexリスト

			//---------------------------------
			// マテリアル
			//---------------------------------
			std::vector<Material>	materials = {};						// 一覧

			//---------------------------------
			// アニメーション
			//---------------------------------
			std::vector<std::shared_ptr<AnimationData>> animations = {};	// アニメーションデータリスト
		};
	}
}
