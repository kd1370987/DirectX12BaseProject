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
}

namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class ShapeRenderer;
	class RenderContext;
	class RenderPassRegistry;
	class MeshBufferAllocator;

	// グラフィックスエンジンの初期化に必要な情報
	struct GraphicsEngineDesc
	{
		UINT width = 0;						// ウィンドウの横幅
		UINT height = 0;					// ウィンドウの縦幅

		D3D12::PipelineStateManager* pPipelineStateManager = nullptr;
	};

	// 64ビットのソートキー
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

		// インスタンスデータ
		bool isAnimation = false;

		// 構造体インデックス
		UINT instnaceIndex = 0;
		UINT subsetIndex = 0;

		// メッシュシェーダー用インデックス
		UINT meshInstanceIndex = 0;
		UINT meshMaterialIndex = 0;
		MeshAllocationHandle meshHandle = {};

		// このサブセットを描画するためのメッシュレット数
		UINT subsetMeshletCount = 0;

		// ヘルパー関数
		uint8_t GetPassIndex()		const { return static_cast<uint8_t>(sortKey.value >> 56); }
		uint8_t GetPSOID()			const { return static_cast<uint8_t>((sortKey.value >> 48) & 0xFF); }
		uint16_t GetMaterialID()	const { return static_cast<uint16_t>((sortKey.value >> 32) & 0xFFFF); }
		uint16_t GetMeshID()		const { return static_cast<uint16_t>((sortKey.value >> 16) & 0xFFFF); }
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
		void BegineFrame();
		void Excute();
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
		// 環境データ
		void SetAmbientData(const AmbientData& a_data);
		const AmbientData& GetAmbientData() const;
		//--------------------------------------------------------------------------------------------
		// 描画コマンド
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
			const RangeHandle<Resource::NodePoseMatrix> nodePoseHnandle
		);

		/// <summary>
		/// 指定したモデルを指定の座標に描画する命令 : 即時実行ではなく、コマンドとしてためたのちに一括で実行される
		/// </summary>
		/// <param name="a_world">ワールド</param>
		/// <param name="a_pModel">モデルのポインタ</param>
		/// <param name="a_worldMatrix">ワールド行列</param>
		/// <param name="a_albedScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Color& a_albedScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = {1,1,1}
		);
		/// <summary>
		/// 指定したモデルを指定の座標に描画する命令 : 即時実行ではなく、コマンドとしてためたのちに一括で実行される
		/// </summary>
		/// <param name="a_world">ワールド</param>
		/// <param name="a_pModel">モデルのポインタ</param>
		/// <param name="a_worldMatrix">ワールド行列</param>
		/// <param name="a_prevMatrix">過去ワールド行列</param>
		/// <param name="a_albedScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Matrix& a_prevMatrix,
			const DXSM::Color& a_albedScale = Color::WHITE,
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
		/// <param name="a_albedScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Matrix& a_prevMatrix,
			const RangeHandle<Resource::BoneMatrix>& a_boneHandle,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodePoseHandle,
			const DXSM::Color& a_albedScale = Color::WHITE,
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

	private:

		// カメラをGPU用データに変換
		void CreateGPUCameraData();

		// レイトレ用BLAS初期化
		void ProcessInitQueue(D3D12::Device* a_pDevice, D3D12::GraphicsCommandList* a_pCmdList);


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
		// 描画命令
		//--------------------------------------------------------------------------------------------
		// ソートキー持ち描画コマンドリスト
		std::vector<LightWeightDrawItem> m_lightWeightDrawItemVec = {};

		std::vector<Raytracing::DynamicRaytracingRequest> m_dynamicRayRequestVec = {};

		// GPUスキニング配列
		std::vector<SkinningDispatchItem> m_skinningDispathItemVec = {};
	};
}