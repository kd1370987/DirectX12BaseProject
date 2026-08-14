#pragma once
#include "CBData.h"

namespace Engine
{
	namespace D3D12
	{
		class GraphicsPSOManager;
		class RootSignatureManager;

		class PipelineStateManager;
	}

	namespace ECS
	{
		class World;
	}

	namespace Resource
	{
		class Mesh;
		class Material;
		class ShadingModelTable;
		struct ModelDrawCommand;
	}
}

namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class ShapeRenderer;
	class RenderContext;
	class RenderPassRegistry;
	class MeshBufferAllocator;
	struct PSOKey;

	// グラフィックスエンジンの初期化に必要な情報
	struct GraphicsEngineDesc
	{
		UINT width = 0;						// ウィンドウの横幅
		UINT height = 0;					// ウィンドウの縦幅

		D3D12::PipelineStateManager* pPipelineStateManager = nullptr;
	};

	// 64ビットのソートキー
	//
	// ここに詰まっている ID は「並べ替えと、同じ状態をまとめるため」だけのもの。
	// 世代を持たないので、ここからリソースを引かないこと。
	// 実体が必要なときは LightWeightDrawItem のハンドルから取得する
	union RenderSortKey
	{
		uint64_t value;
		struct {
			// 下位ビットから順に判断優先度が低くなるように配置する
			uint64_t depth : 16;			// 深度
			uint64_t meshID : 16;			// メッシュ
			uint64_t materialID : 16;		// マテリアル
			uint64_t psoID : 8;				// PSOID
			uint64_t passIndex : 8;			// パスインデックス
		} bits;
	};

	struct LightWeightDrawItem
	{
		// 描画順序と各種IDの情報すべてを持つ
		RenderSortKey sortKey;
		UINT subIndex = 0;

		// 描画に使うリソース。
		// 実体はリソースマネージャーに置いたままにして、ここではハンドルだけを持つ。
		// 描画する瞬間に引き直すこと
		Handle<Resource::Mesh>		meshHandle = {};
		Handle<Resource::Material>	materialHandle = {};

		// インスタンスデータ
		bool isAnimation = false;

		// 構造体インデックス
		UINT instanceIndex = 0;
		UINT subsetIndex = 0;

		// メッシュシェーダー用インデックス
		UINT meshInstanceIndex = 0;
		UINT meshMaterialIndex = 0;

		// このサブセットを描画するためのメッシュレット数
		UINT subsetMeshletCount = 0;

		// ヘルパー関数
		uint8_t GetPassIndex()		const { return static_cast<uint8_t>(sortKey.value >> 56); }
		uint8_t GetPSOID()			const { return static_cast<uint8_t>((sortKey.value >> 48) & 0xFF); }
	};

	/// <summary>
	/// GPUスキニングするエンティティの命令
	/// </summary>
	struct SkinningDispatchItem
	{
		RangeHandle<Resource::MeshVertexFloat> staticVertexHandle;		// アセット側の頂点データ
		RangeHandle<uint32_t> staticIndexHandle;						// アセット側のインデックスデータ
		RangeHandle<Resource::NodePoseMatrix> nodePoseMat;				// CPUで更新されたボーンノード行列
		RangeHandle<Resource::MeshVertexFloat> animatedHandle;
		RangeHandle<Resource::BoneMatrix> boneHandle;					// ボーン行列

		// 自身のBLASと変形後頂点を入れるメガバッファのハンドルを保持しているインスタンスのハンドル
		Handle<Raytracing::DynamicRaytracingData> animHandle;
	};

	// グラフィックスエンジン
	class GraphicsEngine
	{
	public:

		GraphicsEngine();
		~GraphicsEngine();

		// 初期化・解放
		void Init(D3D12::GraphicsCommandList* a_pCmdList, const GraphicsEngineDesc& a_desc);
		void Release();

		
		// フレームの開始・終了処理
		void BeginFrame();
		void Execute();
		void EndFrame();

		// アクセサ
		const Graphics::RenderContext* GetRenderContext() const;
		Graphics::RenderContext* RefRenderContext();
		D3D12::PipelineStateManager* RefPipelineStateManager();
		Graphics::RenderPassRegistry* RefRenderPassRegistry();

		RenderGraph* RefRenderGraph();

		//--------------------------------------------------------------------------------------------
		// GPU送信用データ
		//--------------------------------------------------------------------------------------------
		// カメラ
		void SetCameraMat(const DXSM::Matrix& a_worldMat);
		void SetProjMat(const DXSM::Matrix& a_projMat);

		const CameraData& GetCameraData() const;
		const CameraData& GetGPUCameraData() const;
		const CameraData& GetCPUCameraData() const;

		// カメラの割り込み(エディターカメラなど)
		// ECS側のカメラ設定は Execute() 内の Draw フェーズ(PreDraw)で行われるため、
		// 単に SetCameraMat を先に呼んでも上書きされてしまう。
		// ここに積んでおくと、ECS側の設定が終わった後・GPUデータ作成の直前に適用される。
		void SetCameraOverride(const DXSM::Matrix& a_worldMat, const DXSM::Matrix& a_projMat);
		void ClearCameraOverride();
		// 環境データ
		void SetAmbientData(const AmbientData& a_data);
		const AmbientData& GetAmbientData() const;
		AmbientData& RefAmbientData();
		//--------------------------------------------------------------------------------------------
		// 計算コマンド : スキニング
		//--------------------------------------------------------------------------------------------
		
		/// <summary>
		/// GPUスキニングさせる命令
		/// </summary>
		/// <param name="a_world">ECSワールドポインタ</param>
		/// <param name="a_pModel">モデルポインタ</param>
		/// <param name="dynamicHandle">変形後のデータを入れるインスタンス</param>
		/// <param name="nodePoseHnandle">ノード行列</param>
		void SubmitSkinning(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const Handle<Raytracing::DynamicRaytracingData> dynamicHandle,
			const RangeHandle<Resource::NodePoseMatrix> nodePoseHnandle,
			const RangeHandle<Resource::BoneMatrix> boneHandle
		);

		//--------------------------------------------------------------------------------------------
		// 描画コマンド : モデル
		//--------------------------------------------------------------------------------------------

		/// <summary>
		/// 指定したモデルを指定の座標に描画する命令 : 即時実行ではなく、コマンドとしてためたのちに一括で実行される
		/// </summary>
		/// <param name="a_world">ワールド</param>
		/// <param name="a_pModel">モデルのポインタ</param>
		/// <param name="a_worldMatrix">ワールド行列</param>
		/// <param name="a_albedoScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Color& a_albedoScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = {1,1,1}
		);
		/// <summary>
		/// 指定したモデルを指定の座標に描画する命令 : 即時実行ではなく、コマンドとしてためたのちに一括で実行される
		/// </summary>
		/// <param name="a_world">ワールド</param>
		/// <param name="a_pModel">モデルのポインタ</param>
		/// <param name="a_worldMatrix">ワールド行列</param>
		/// <param name="a_prevMatrix">過去ワールド行列</param>
		/// <param name="a_albedoScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Matrix& a_prevMatrix,
			const DXSM::Color& a_albedoScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = { 1,1,1 }
		);
		/// <summary>
		/// 指定したモデルを指定の座標に描画する命令 : 即時実行ではなく、コマンドとしてためたのちに一括で実行される
		/// </summary>
		/// <param name="a_world">ワールド</param>
		/// <param name="a_pModel">モデルのポインタ</param>
		/// <param name="a_worldMatrix">ワールド行列</param>
		/// <param name="a_prevMatrix">過去ワールド行列</param>
		/// <param name="a_boneHandle">ボーン行列配列ハンドル</param>
		/// <param name="a_nodePoseHandle">スケルトンポーズ行列配列ハンドル</param>
		/// <param name="a_animData">アニメーション後頂点配列</param>
		/// <param name="a_albedoScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Matrix& a_prevMatrix,
			const RangeHandle<Resource::BoneMatrix>& a_boneHandle,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodePoseHandle,
			const Handle<Raytracing::DynamicRaytracingData>& a_animData,
			const DXSM::Color& a_albedoScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = { 1,1,1 }
		);

		/// <summary>
		/// レイトレワールドに登録するアニメーションモデル
		/// </summary>
		/// <param name="a_worldMat">ワールド行列</param>
		/// <param name="a_colorScale">色スケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		/// <param name="dynamicHandle">ダイナミックリソースハンドル</param>
		/// <param name="nodePoseHnandle">ノードポーズハンドル</param>
		void SubmitModel(
			const DXSM::Matrix& a_worldMat,				// ワールド行列
			const DXSM::Vector4& a_colorScale,			// 色スケール
			const DXSM::Vector3& a_emissiveScale,		// エミッシブスケール
			const Engine::Handle<Raytracing::DynamicRaytracingData> dynamicHandle,
			const Engine::Handle<Resource::NodePoseMatrix> nodePoseHnandle
		);

		//--------------------------------------------------------------------------------------------
		// 描画コマンド : UI
		//--------------------------------------------------------------------------------------------
		/// <summary>
		/// UI描画命令。座標系はピクセル(左上原点/Y下向き)。回転・アスペクト補正・
		/// ピボットはエンジン側でピクセル空間で計算するため、斜め回転でも歪まない。
		/// </summary>
		/// <param name="a_texHandle">テクスチャハンドル</param>
		/// <param name="a_pixelPos">ピボットのスクリーン座標(px, 左上原点)</param>
		/// <param name="a_pixelSize">表示フルサイズ(px)</param>
		/// <param name="a_color">色</param>
		/// <param name="a_rotationDeg">回転(度, 時計回り)</param>
		/// <param name="a_layer">Z順</param>
		/// <param name="a_uvOffset">UVオフセット</param>
		/// <param name="a_pivot">回転軸/基準点(正規化[0,1], 0.5=中心)</param>
		void SubmitUI(
			const Handle<Resource::Texture>& a_texHandle,
			const DXSM::Vector2& a_pixelPos,
			const DXSM::Vector2& a_pixelSize,
			const DXSM::Vector4& a_color = {},
			float a_rotationDeg = 0,
			float a_layer = 0,
			const DXSM::Vector2& a_uvOffset = {},
			const DXSM::Vector2& a_pivot = { 0.5f, 0.5f }
		);

		/// <summary>
		/// UI描画命令(サイズはテクスチャ原寸×スケール)。座標系はピクセル。
		/// </summary>
		/// <param name="a_texHandle">テクスチャハンドル</param>
		/// <param name="a_pixelPos">ピボットのスクリーン座標(px, 左上原点)</param>
		/// <param name="a_scale">テクスチャ原寸に掛けるスケール</param>
		/// <param name="a_color">色</param>
		/// <param name="a_rotationDeg">回転(度, 時計回り)</param>
		/// <param name="a_layer">Z順</param>
		/// <param name="a_uvOffset">UVオフセット</param>
		/// <param name="a_pivot">回転軸/基準点(正規化[0,1], 0.5=中心)</param>
		void SubmitUI(
			const Handle<Resource::Texture>& a_texHandle,
			const DXSM::Vector2& a_pixelPos,
			float a_scale = 1.0f,
			const DXSM::Vector4& a_color = Color::WHITE,
			float a_rotationDeg = 0,
			float a_layer = 0,
			const DXSM::Vector2& a_uvOffset = {},
			const DXSM::Vector2& a_pivot = { 0.5f, 0.5f }
		);

		// 追加
		UINT SetInstanceData(const InstanceData& a_instanceData);
		UINT SetInstanceData(const MeshInstanceData& a_instanceData);
		UINT SetSubSetData(const SubSetData& a_subsetData);
		UINT SetMeshMaterialData(const MeshMaterial& a_subsetData);
		void AddItem(const LightWeightDrawItem& a_item);

		// 取得
		std::span<const LightWeightDrawItem> GetPassItems(uint8_t a_passIndex);

		// パスの描画実行
		void DrawQueue(Graphics::RenderContext* a_pCtx, uint8_t a_passIndex);
		void BindPSO(Graphics::RenderContext* a_pCtx, uint8_t a_psoIndex);
		// 配列取得
		const std::vector<SkinningDispatchItem>& GetSkinningImtes() const { return m_skinningDispathItemVec; }

		// バッファ取得
		MeshBufferAllocator* RefMeshBufferAllocator() { return m_upMeshBufferAllocator.get(); }

		const std::vector<UIData>& GetUIDataBuffer() { return m_uiDrawItemVec; }

	private:

		// カメラをGPU用データに変換
		void CreateGPUCameraData();

		// レイトレ用BLAS初期化
		void ProcessInitQueue(D3D12::Device* a_pDevice, D3D12::GraphicsCommandList* a_pCmdList);

		// テクスチャハンドルからSRVのインデックスを取得する
		int GetSRVIndexFromTextureHandle(const Handle<Resource::Texture>& a_texHandle);

		//--------------------------------------------------------------------------------------------
		// SubmitModel / SubmitUI 共通処理(重複コードの関数化)
		//--------------------------------------------------------------------------------------------

		// 描画コマンドからメッシュ・マテリアル・シェーディングモデルをまとめて取得する。
		// いずれかが取得できなければ false(呼び出し側はスキップする)。
		bool FetchDrawResources(
			const Resource::ModelDrawCommand& a_cmd,
			const Resource::Mesh*& a_pOutMesh,
			const Resource::Material*& a_pOutMaterial,
			const Resource::ShadingModelTable*& a_pOutShadingModel);

		// マテリアルとスケールからメッシュシェーダー用マテリアルデータを構築する。
		MeshMaterial BuildMeshMaterial(
			const Resource::Material* a_pMaterial,
			const DXSM::Color& a_albedoScale,
			const DXSM::Vector3& a_emissiveScale);

		// 1つの描画コマンドを、シェーディングモデルが持つ全パスへ登録する共通処理。
		// (メッシュシェーダー用データ構築・PSO要求・描画アイテム登録をまとめて行う)
		void RegisterDrawCommandToPasses(
			const Resource::ModelDrawCommand& a_cmd,
			const Resource::Mesh* a_pMesh,
			const Resource::Material* a_pMaterial,
			const Resource::ShadingModelTable* a_pShadingModel,
			const DXSM::Matrix& a_mat,
			const DXSM::Matrix& a_prevMat,
			uint32_t a_instanceIdx,
			uint32_t a_subsetIdx,
			bool a_isAnimation,
			uint32_t a_animatedVertexStart,
			const DXSM::Color& a_albedoScale,
			const DXSM::Vector3& a_emissiveScale,
			PSOKey a_psoKey);

		// ピクセル空間で回転・アスペクト補正・ピボットを解決し、UIData(NDC基底)を
		// 1件バッファへ積む(SubmitUI 各オーバーロード共通)。
		void PushUIData(
			uint32_t a_texIndex,
			const DXSM::Vector2& a_pixelPos,
			const DXSM::Vector2& a_pixelSize,
			const DXSM::Vector4& a_color,
			float a_rotationDeg,
			float a_layer,
			const DXSM::Vector2& a_uvOffset,
			const DXSM::Vector2& a_pivot);
	private:
		//--------------------------------------------------------------------------------------------
		// 主要クラス
		//--------------------------------------------------------------------------------------------
		// レンダーコンテキスト : 一フレーム内の描画情報を扱う
		std::vector<std::unique_ptr<RenderContext>> m_upRenderContextVec = {};
		UINT m_currentFrameIndex = 0;

		// PSOやルートシグネチャの管理
		D3D12::PipelineStateManager* m_pPipelineStateManager = nullptr;

		// 形状描画クラス
		std::unique_ptr<ShapeRenderer> m_upShapeRender = nullptr;

		// レンダーパスの登録場所
		std::unique_ptr<RenderPassRegistry> m_upRenderPassRegistry = nullptr;

		// レンダーグラフ
		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;

		//メッシュバッファ管理
		std::unique_ptr<MeshBufferAllocator> m_upMeshBufferAllocator = nullptr;
	
		//--------------------------------------------------------------------------------------------
		// GPU送信用データ
		//--------------------------------------------------------------------------------------------
		// カメラデータ
		CameraData m_cbCamera = {};
		CameraData m_cbGPUCamera = {};

		// カメラの割り込み用
		bool m_isCameraOverride = false;
		DXSM::Matrix m_cameraOverrideWorldMat = DXSM::Matrix::Identity;
		DXSM::Matrix m_cameraOverrideProjMat = DXSM::Matrix::Identity;
		DXSM::Matrix m_prevViewMat = {};
		DXSM::Matrix m_prevProjMat = {};
		DXSM::Matrix m_prevNonJitteredViewProj = {};
		int m_totlaFrameCount = 0;

		// 環境データ
		AmbientData m_cbAmbient = {};
		
		// オブジェクト単位データ
		std::vector<InstanceData> m_instanceDataVec = {};
		std::vector<MeshInstanceData> m_meshInstanceDataVec = {};

		// サブセット単位データ
		std::vector<SubSetData> m_subSetDataVec = {};
		std::vector<MeshMaterial> m_meshMaterialDataVec = {};

		//--------------------------------------------------------------------------------------------
		// 命令配列
		//--------------------------------------------------------------------------------------------
		// ソートキー持ち描画コマンドリスト
		std::vector<LightWeightDrawItem> m_lightWeightDrawItemVec = {};

		// UI用アイテム配列
		std::vector<UIData> m_uiDrawItemVec = {};

		// GPUスキニング配列
		std::vector<SkinningDispatchItem> m_skinningDispathItemVec = {};

		// アニメーション用レイトレインスタンス作成命令
		std::vector<Raytracing::DynamicRaytracingRequest> m_dynamicRayRequestVec = {};

	};
}