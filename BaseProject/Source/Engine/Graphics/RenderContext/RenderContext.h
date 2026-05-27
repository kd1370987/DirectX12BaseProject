#pragma once


class CBAllocater;

namespace Engine::Resource
{
	class QuadPolygon;
}

namespace Engine::D3D12
{
	class CommandList;
	class RootSignature;
}

namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class ShapeRenderer;

	// 64ビットのソートキー
	union RenderSortKey
	{
		uint64_t value;
		struct {
			// 下位ビットから順に判断優先度が低くなるように配置する
			uint64_t depth : 16;			// 深度
			uint64_t meshID : 16;			// メッシュ
			uint64_t materialID : 16;		// マテリアル
			uint64_t psoID: 8;				// PSOID
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
		Storage::Range boneRange = {};

		DirectX::XMFLOAT4X4 worldMat = {};
		DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
		DirectX::XMFLOAT3	emissiveScale = { 1,1,1 };

		// ヘルパー関数
		uint8_t GetPassIndex()		const { return static_cast<uint8_t>(sortKey.value >> 56); }
		uint8_t GetPSOID()			const { return static_cast<uint8_t>((sortKey.value >> 48) & 0xFF); }
		uint16_t GetMaterialID()	const { return static_cast<uint16_t>((sortKey.value >> 32) & 0xFFFF); }
		uint16_t GetMeshID()		const { return static_cast<uint16_t>((sortKey.value >> 16) & 0xFFFF); }
	};

	struct DrawItem2D
	{
		Resource::Handle<D3D12::SRV> srvHandleRange = {};

		DirectX::XMFLOAT4X4 worldMat = {};
		DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
	};

	struct DebugDrawInfo
	{
		UINT startIndex;		// インデックスバッファの開始位置
	};



	// レンダーコンテキスト作成時に必要な情報
	struct RenderContextDesc
	{
		// D3Dオブジェクトのキャッシュ
		ID3D12Device* pDevice = nullptr;

		// クラスのキャッシュ
		ShapeRenderer*					pShapeRender	= nullptr;

		// アロケーターのメモリ容量
		size_t cbAllocatorMemSize = 32 * 1024 * 1024;

		// ボーン用行列数
		UINT boneElementNum = 0;
	};

	// マイフレームリセットするときに外部からもらう情報
	struct FrameDesc
	{
		ID3D12GraphicsCommandList* pCmdList = nullptr;
		D3D12::CommandList* pCmdListClass = nullptr;
	};
	
	// 現在のフレームの描画管理クラス
	class RenderContext
	{
	public:

		struct BonePallete
		{
			DirectX::XMFLOAT4X4 mat;
		};

		//--------------------------------------------------------------------------------------------
		// クラス基盤
		//--------------------------------------------------------------------------------------------
		RenderContext();
		~RenderContext();

		// 初期化・解放
		void Init(const RenderContextDesc& a_desc);

		// フレームの初めに呼ぶ
		void Begine(const FrameDesc& a_desc);
		void Clear();

		ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() const;

		// 現在のコマンドリストを取得
		D3D12::CommandList* GetCurrentCmdList();

		//--------------------------------------------------------------------------------------------
		// カメラ関係
		//--------------------------------------------------------------------------------------------
		// 描画時情報セット
		CBCamera& RefCBCamera() { return m_cb0_camera; }
		void SetToShader(const DirectX::XMFLOAT4X4& a_worldMat);		// ワールド行列
		void SetProjectionMatrix(DirectX::XMFLOAT4X4 a_projMat);		// プロジェクション行列

		// 現在の描画しているカメラの情報を取得
		float GetCameraAspectRate();									// アスペクトレート
		const DirectX::XMFLOAT4X4& GetCameraRotMat();					// 回転行列
		const DXSM::Vector3& GetCameraPOS();							// 座標
		const CBCamera& GetCamera();

		// 描画直前にカメラの情報をGPU側に送る
		void BindCameraCB();
		void BindAmbientCB();

		//--------------------------------------------------------------------------------------------
		// バッファ関係
		//--------------------------------------------------------------------------------------------
		// 現在のフレームの定数バッファアロケーターにアクセス
		CBAllocater* BindCB();

		// 定数バッファをルートでバインド
		void BindRootCBV(UINT a_index,const void* a_pData,size_t a_size);
		template<typename T>
		void BindRootCBV(UINT a_index,const T& a_data);

		// レンダーターゲットの切り替え
		// 基本的にハンドルで管理しているため内部以外では直接触らない
		void ChangeRenderTarget(
			const std::vector<Resource::Handle<D3D12::RTV>>& a_rtvHandleVec,
			const Resource::Handle<D3D12::DSV>& a_dsvHandle
		);

		// テクスチャハンドルからSRVをバインドする
		void BindSRV(UINT a_rootIdx, std::vector<Resource::Handle<Resource::Texture>>& a_texHandles);

		// SRVハンドルをもらってコピーする
		void BindSRV(UINT a_rootIdx, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHandles);
		void BindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle);
		void BindSRV(UINT a_rootIdx,Resource::Handle<D3D12::SRV> a_srvHandle);

		void ComputeBindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle);

		// UAV
		void BindUAV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		void BindUAVBindLess(UINT a_rootIdx, Resource::Handle<D3D12::UAV> a_handle);

		// 直接GPUアドレスを取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleBindLess(Resource::Handle<D3D12::SRV> a_handle);

		// レンダーターゲットのクリア
		void ClearRenderTarget(const Resource::Handle<Resource::Texture>& a_texHandle);

		// 深度値バッファのクリア
		void ClearDSV(const Resource::Handle<D3D12::DSV>& a_DSVHandle);

		// 矩形描画のためのクラス取得
		ShapeRenderer* RefShapeDraw();

		// ヒープのセット
		void BindHeap();
		void BindCopyHeapAndSumpler();
		void BindCopyHeapAndSumplerBindLess();
		void BindHeaps(UINT a_numHeaps, ID3D12DescriptorHeap *const* a_pHeaps);

		// バインドボーン
		// すべてのアニメーション行列を配置しているから一括で送れる
		// 定数バッファでスタートインデックスとカウントを送る必要あり
		void BindSRVBone();
		void BindCBBone(const Storage::Range& a_range);

		//--------------------------------------------------------------------------------------------
		// 描画コマンド
		//--------------------------------------------------------------------------------------------
		// 描画命令の追加
		void AddItem(RenderQueueType2D a_type, const DrawItem2D& a_itemVec);		// 2D
		void AddItem(const LightWeightDrawItem& a_item) { m_lightWeightDrawItemVec.push_back(a_item); }

		// 描画命令の取得
		const std::vector<DrawItem2D>& GetItemVec(const RenderQueueType2D& a_type) const;	// 2D

		std::span<const LightWeightDrawItem> GetPassItems(uint8_t a_passIndex);
		
		// 描画命令の実行
		void Excute(RenderGraph* a_pGraph);

		// 描画命令のコマンドリスト削除
		void ClearCmd();

		//--------------------------------------------------------------------------------------------
		// 描画パス構築
		//--------------------------------------------------------------------------------------------
		// グラフィックスルートシグネチャをセット、前回と変更がない場合はスキップ
		void SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig);

		// パイプラインステートをセット、前回と変更がない場合はスキップ
		void SetGraphicPSO(ID3D12PipelineState* a_pPSO);

		// プリミティブトポロジーセット
		void SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri);

		// 1Draw当たりのオブジェクトに対する定数
		void BindObuje(
			UINT a_index,
			const DirectX::XMFLOAT2& a_uv = { 0.0f,0.0f },
			const DirectX::XMFLOAT2& a_tile = { 1.0f,1.0f }
		);

		// マテリアルをSRVとして送信、その際にマテリアルの定数も送信
		void BindMaterial(
			UINT a_index,
			const Resource::Material* a_pMaterial,
			const DirectX::XMFLOAT4& a_colorScale,
			const DirectX::XMFLOAT3& a_emissiveScale
		);
		void BindMaterial(
			UINT a_index,
			const uint16_t& a_materialID,
			const DirectX::XMFLOAT4& a_colorScale,
			const DirectX::XMFLOAT3& a_emissiveScale
		);
		void BindMaterialSRV(
			UINT a_index,
			const Resource::Material* a_pMaterial
		);
		void BindMaterialSRV(
			UINT a_index,
			uint16_t a_materialID
		);
		// ルートパラメタインデックスを指定してのメッシュバインド
		void BindMesh(
			UINT a_index,
			const Resource::Mesh* a_pMesh,
			const DirectX::XMFLOAT4X4& a_worldMat
		);
		void BindMeshMat(UINT a_index,const DirectX::XMFLOAT4X4& a_worldMat);
		void BindMesh(uint16_t a_meshID);
		// ビューポート設定
		void SetViewPort();

		// シザーレクト設定
		void SetScissorRect();

		// モデルの描画
		void Draw(const Resource::Mesh* a_pMesh,UINT a_subIdx);
		void Draw(uint16_t a_meshID,UINT a_subIdx);


		//リソースバリア設定
		void Transition(
			ID3D12Resource* a_pResource,
			D3D12_RESOURCE_STATES a_before,
			D3D12_RESOURCE_STATES a_after
		);

		// バックバッファに切り替え
		void ChangeBackBuffer();


		// クワッド描画
		void DrawQuad();

		// UI描画
		void DrawUIQueue(RenderQueueType2D a_type);

		// 形状描画
		void ShapeDraw();


	private:
		// D3DObject
		ID3D12Device* m_pDevice = nullptr;

		// 形状描画クラス
		ShapeRenderer* m_pShapeDraw = nullptr;
		D3D12::DynamicVertexBuffer<Resource::Vertex> m_shapeVertexBuffer;
		

		//--------------------------------------------------------------------------------------------
		// フレーム限定リソース
		//--------------------------------------------------------------------------------------------
		std::unique_ptr<CBAllocater> m_upCBAllocater = nullptr;	// 定数バッファアロケーター
		D3D12::StaticStructuredBuffer<BonePallete> m_boneBuffer;
		D3D12::CommandList* m_pCmdList = nullptr;
		// コピー用ヒープ
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> m_copyHeap = {};				// ラスタライザ用
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_bindLessHeap;	// バインドレス用
		UINT m_currentHeapOffset = 0;



		// 定数バッファ
		CBCamera m_cb0_camera = {};
		float m_aspectRate = 0.0f;

		CBObject m_cb1_object = {};
		CBMeshTrans m_cb2_MeshTrans = {};
		CBMaterial m_cb3_Material = {};
		CBBone m_cb4_Bone = {};
		CBAmbient m_cb5_Ambient = {};
		CBUI m_cbUI = {};
		
		// 2D用描画アイテムキュー
		std::unordered_map<RenderQueueType2D, std::vector<DrawItem2D>> m_drawItem2DMap = {};

		// 描画用ポリゴン
		std::shared_ptr<Resource::QuadPolygon> m_spQuadPolygon = nullptr;

		// ソートキー持ち描画コマンドリスト
		std::vector<LightWeightDrawItem> m_lightWeightDrawItemVec = {};
	};
	template<typename T>
	inline void RenderContext::BindRootCBV(UINT a_index, const T& a_data)
	{
		BindRootCBV(a_index, a_data, sizeof(T));
	}
}