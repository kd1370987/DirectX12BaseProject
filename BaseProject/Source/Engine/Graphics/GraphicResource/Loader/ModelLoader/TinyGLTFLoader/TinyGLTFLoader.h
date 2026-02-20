#pragma once

//=========================================================
// GLTFマテリアルデータ
//=========================================================
struct GLTFMaterial
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
struct MeshVertex8bit;
struct MeshFace;
struct MeshSubset;

struct GLTFNode
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
		std::vector<MeshVertex8bit>	vertices = {};			// 頂点配列
		std::vector<MeshFace>	faces = {};				// 面情報配列
		std::vector<MeshSubset> subsets = {};			// サブセット配列

		bool					isSkinMesh = false;	// スキンメッシュはあるかどうか
	};
	NodeMesh					nodeMesh = {};
};

//=========================================================
// GLTFアニメーションデータ
//=========================================================
struct AnimationNode;

struct GLTFAnimationData
{
	//---------------------------------
	// 基本情報
	//---------------------------------
	std::string									name = "none";					// 名前
	float										maxLength = 0;			// アニメーションの長さ
	std::vector<std::shared_ptr<AnimationNode>> spAnimationNodes = { nullptr };		// 全ノード用アニメーションデータ
};

//=========================================================
// モデルデータ(一つのGLTFに対してついになるもの,中間素材)
//=========================================================
struct GLTFModel
{
	//---------------------------------
	// ノードデータ
	//---------------------------------
	std::vector<GLTFNode>		nodes = {};							// 全ノードデータ
	std::vector<int>			rootNodeIndices = {};				// ルートノードのみのIndexリスト
	std::vector<int>			boneNodeIndices = {};				// ボーンノードのみのIndexリスト

	//---------------------------------
	// マテリアル
	//---------------------------------
	std::vector<GLTFMaterial>	materials = {};						// 一覧

	//---------------------------------
	// アニメーション
	//---------------------------------
	std::vector<std::shared_ptr<GLTFAnimationData>> animations = {};	// アニメーションデータリスト
};

//=========================================================
// 
// GLTF,GLB モデル読み込み専用
// 
//=========================================================

namespace Load
{
	std::shared_ptr<GLTFModel> Model(std::string_view a_filePath);
}