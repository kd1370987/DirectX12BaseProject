#pragma once
namespace Engine::Resource::Parse
{
	//====================================================================================================
	// 中間素材
	// .gltf .obj .fbx ... などから読み込んだものを独自の構造体に入れる前の中間素材。
	// Zミラーなどの処理に渡すためのものなので、読み込んだままの情報が入る
	//
	// この層はフォーマット非依存・エンジンのGPUリソース非依存であることを保つこと。
	// 座標系の変換は ModelProcessor が担当するため、パーサ側では一切行わない。
	//====================================================================================================

	/// <summary>
	/// パースされたマテリアル情報
	/// </summary>
	struct RawMaterial
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

	/// <summary>
	/// パースされたメッシュ情報
	/// </summary>
	struct RawMesh
	{
		std::vector<Engine::Resource::MeshVertex8bit>	vertices = {};		// 頂点配列
		std::vector<Engine::Resource::MeshFace>			faces = {};			// 面情報配列
		std::vector<Engine::Resource::MeshSubset>		subsets = {};		// サブセット配列

		bool				isSkinMesh = false;								// スキンメッシュはあるかどうか
	};

	/// <summary>
	/// パースされたノード情報
	/// </summary>
	struct RawNode
	{
		//---------------------------------
		// 基本情報
		//---------------------------------
		std::string				name = "none";				// 名前
		std::vector<int>		children = {};				// 子Indexリスト
		int						parent = -1;				// 親のIndex
		int						boneNodeIndex = -1;			// ボーンの場合のIndex

		DirectX::XMFLOAT4X4		localTransform = {};		// ローカル行列	中心点からの座標
		DirectX::XMFLOAT4X4		worldTransform = {};		// ワールド行列 中心点
		DirectX::XMFLOAT4X4		inverseBindMatrix = {};		// ボーンのオフセット行列

		//---------------------------------
		// Mesh専用
		//---------------------------------
		int						meshIndex = -1;				// RawModel::meshes へのIndex(-1ならメッシュなし)

		bool HasMesh() const { return meshIndex >= 0; }
	};

	//==========================================================
	// アニメーションノード
	// キー自体はエンジン側と同じ表現でよいため既存型を再利用する
	//==========================================================
	struct RawAnimationNode
	{
		int													nodeOffset = -1;	// 対象ノードのオフセット

		// 各チャンネル
		std::vector<Engine::Resource::AnimationKeyXMFLOAT3>	translations = {};	// 座標キーリスト
		std::vector<Engine::Resource::AnimationKeyQuaternion> rotations = {};	// 回転キーリスト
		std::vector<Engine::Resource::AnimationKeyXMFLOAT3>	scales = {};		// 拡縮キーリスト
	};

	/// <summary>
	/// パースされたアニメーション情報
	/// </summary>
	struct RawAnimation
	{
		//---------------------------------
		// 基本情報
		//---------------------------------
		std::string						name = "none";			// 名前
		float							maxLength = 0;			// アニメーションの長さ
		std::vector<RawAnimationNode>	animationNodes = {};	// 全ノード用アニメーションデータ
	};

	//=========================================================
	// モデルデータ(一つのGLTFに対してついになるもの,中間素材)
	//=========================================================
	struct RawModel
	{
		//---------------------------------
		// ノードデータ
		//---------------------------------
		std::vector<RawMaterial>	materials = {};			// マテリアル
		std::vector<RawMesh>		meshes = {};			// メッシュ
		std::vector<RawAnimation>	animations = {};		// アニメーションデータリスト
		std::vector<RawNode>		nodes = {};				// 全ノードデータ

		std::vector<int>			rootNodeIndices = {};	// ルートノードのみのIndexリスト
		std::vector<int>			boneNodeIndices = {};	// ボーンノードのみのIndexリスト
	};
}
