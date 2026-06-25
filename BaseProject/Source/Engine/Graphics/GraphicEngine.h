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
}

namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class ShapeRenderer;
	class RenderContext;

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

		// ヘルパー関数
		uint8_t GetPassIndex()		const { return static_cast<uint8_t>(sortKey.value >> 56); }
		uint8_t GetPSOID()			const { return static_cast<uint8_t>((sortKey.value >> 48) & 0xFF); }
		uint16_t GetMaterialID()	const { return static_cast<uint16_t>((sortKey.value >> 32) & 0xFFFF); }
		uint16_t GetMeshID()		const { return static_cast<uint16_t>((sortKey.value >> 16) & 0xFFFF); }
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

		RenderGraph* RefRenderGraph();



		//--------------------------------------------------------------------------------------------
		// GPU送信用データ
		//--------------------------------------------------------------------------------------------
		// カメラ
		void SetCameraMat(const DXSM::Matrix& a_worldMat);
		void SetProjMat(const DXSM::Matrix& a_projMat);

		const CameraData& GetCameraData() const;
		const CameraData& GetGPUCameraData() const;
		// 環境データ
		void SetAmbientData(const AmbientData& a_data);
		const AmbientData& GetAmbientData() const;
		//--------------------------------------------------------------------------------------------
		// 描画コマンド
		//--------------------------------------------------------------------------------------------
		// 追加
		UINT SetInstanceData(const InstanceData& a_instanceData);
		UINT SetSubSetData(const SubSetData& a_subsetData);
		void AddItem(const LightWeightDrawItem& a_item);

		// 取得
		std::span<const LightWeightDrawItem> GetPassItems(uint8_t a_passIndex);

		// パスの描画実行
		void DrawQueue(Graphics::RenderContext* a_pCtx, uint8_t a_passIndex);
		void BindPSO(Graphics::RenderContext* a_pCtx, uint8_t a_psoIndex);

	private:

		// カメラをGPU用データに変換
		void CreateGPUCameraData();


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

		// レンダーグラフ
		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;
	
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

		// サブセット単位データ
		std::vector<SubSetData> m_subSetDataVec = {};

		//--------------------------------------------------------------------------------------------
		// 描画命令
		//--------------------------------------------------------------------------------------------
		// ソートキー持ち描画コマンドリスト
		std::vector<LightWeightDrawItem> m_lightWeightDrawItemVec = {};
	};
}