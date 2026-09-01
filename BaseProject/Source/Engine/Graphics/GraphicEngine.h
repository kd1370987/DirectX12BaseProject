#pragma once
#include "CBData.h"
#include "LightManager/LightManager.h"

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
		class QuadPolygon;
		struct ModelDrawCommand;
	}
}

namespace Engine::Graphics
{
	// 前方宣言
	class RenderContext;
	class MeshBufferAllocator;
	struct PSOKey;

	// レンダリングパイプライン。
	// 設計図(RenderingPipelineAsset)とカメラごとの実行インスタンスに分かれている
	namespace Pipeline
	{
		class PassMetaRegistry;
		class GraphicsPipeline;
		class RenderingPipelineAsset;
		class RenderGraph;
		class Pass;
	}

	//==========================================================================================
	// カメラ1台ぶんの描画要求
	//
	// 毎フレーム積む。積まれなかったカメラは消えたものとして、
	// フレームの終わりに実行インスタンスごと捨てる
	//==========================================================================================
	struct CameraSubmitDesc
	{
		// このカメラを識別する鍵。
		// エンティティの添字はワールドごとに振り直されるので、ワールドと組で持つ
		const ECS::World* pWorld = nullptr;
		uint32_t entity = 0;

		// 使う描画構成(設計図)
		Handle<Pipeline::RenderingPipelineAsset> pipelineHandle = {};

		DXSM::Matrix worldMat = DXSM::Matrix::Identity;
		DXSM::Matrix projMat = DXSM::Matrix::Identity;

		// 0 なら画面の描画解像度に追従する
		UINT viewportWidth = 0;
		UINT viewportHeight = 0;

		int order = 0;			// 小さいものから回す
		bool isMain = false;	// 画面に出るカメラか
	};

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
		RangeHandle<Resource::BoneMatrix> boneHandle;					// ボーン行列(ワールド内のプール添字)

		// ボーンパレット(GPU)での開始位置。
		// プールの添字はワールドごとに 0 から振り直されるので、
		// 複数のシーンを重ねて描くとそのままでは他シーンのボーンを踏む。
		// アップロード時に付く土台を足した「GPU上の」位置をここに持つ
		uint32_t boneBufferStart = 0;

		// 自身のBLASと変形後頂点を入れるメガバッファのハンドルを保持しているインスタンスのハンドル
		Handle<Raytracing::DynamicRaytracingData> animHandle;

		// この命令を出したワールド。
		// animHandle はワールドごとのプールの鍵なので、引くときは必ずこのワールドから引く。
		// シーンを重ねて描くと命令配列に複数のワールドのものが混ざるため、
		// 「今の一番上のシーン」から引くと他人のプールを鍵違いで探すことになる
		ECS::World* pWorld = nullptr;
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
		//--------------------------------------------------------------------------------------------
		// 生成できるパスの一覧
		//
		// パイプラインアセットは「パスの型ID」しか保存しないので、
		// 読み込むときに実体を作り直すためこの一覧が要る。
		// ResourceBuildContext 経由でローダーへ渡される(ScopedResourceBuild が詰める)
		//--------------------------------------------------------------------------------------------
		Pipeline::PassMetaRegistry* RefPassMetaRegistry();

		//--------------------------------------------------------------------------------------------
		// カメラごとの描画構成(新レンダーグラフ)
		//
		// 従来のレンダーグラフとは並走する。
		// パイプラインを持つカメラだけが新しい経路を通り、自分の最終出力テクスチャへ描く。
		// バックバッファへ出すのは従来経路のままなので、
		// 全パスの移植が済むまで画面の見た目は変わらない
		//--------------------------------------------------------------------------------------------
		// 毎フレーム積む。積まなかったカメラはフレームの終わりに捨てられる
		void SubmitCamera(const CameraSubmitDesc& a_desc);

		// このカメラが描いた絵。モニターへ映したいときはこれを引く
		const Resource::Texture* GetCameraFinalTexture(const ECS::World* a_pWorld, uint32_t a_entity) const;

		//--------------------------------------------------------------------------------------------
		// 設計図のパスに対応する、実際に動いている実行インスタンスのパスを引く
		//
		// エディターが触っているのは設計図(RenderingPipelineAsset)側のパスで、
		// これは一度も実行されない = リソースの実体を持たない。
		// モニターのように「今フレーム流れている中身」をノードに出すパスは、
		// ここを通して実行インスタンス側の自分を借りてくる。
		//
		// GUIDは BuildFrom の複製で引き継がれるので、これが設計図と実行を結ぶ鍵になる。
		// 同じ設計図を複数のカメラが使っていればメインカメラのものを返す
		//--------------------------------------------------------------------------------------------
		Pipeline::Pass* FindPipelinePass(const Engine::GUID& a_passGUID) const;

		//--------------------------------------------------------------------------------------------
		// 直近に画面を作っていたカメラの描画構成
		//
		// ゲームのシーンを止めて別のワールドを描く画面(エフェクトエディター)が、
		// 「本番と同じ絵作り」で見るために借りる。
		// 一度もカメラが積まれていなければ無効ハンドル
		//--------------------------------------------------------------------------------------------
		const Handle<Pipeline::RenderingPipelineAsset>& GetLastMainPipelineHandle() const { return m_lastMainPipelineHandle; }

		//--------------------------------------------------------------------------------------------
		// エディター用 : 今フレーム回っているカメラのグラフを順に見る
		//
		// リソースの中身を覗くパネルが使う。
		// 実行インスタンスはカメラごとにあるので、どのカメラのものかが分かるよう
		// 表示用の名前を添えて返す
		//--------------------------------------------------------------------------------------------
		struct PipelineGraphView
		{
			std::string name = {};								// 表示名(設計図の名前 + メインかどうか)
			const Pipeline::RenderGraph* pGraph = nullptr;		// そのカメラの実行グラフ
		};
		std::vector<PipelineGraphView> CollectPipelineGraphs() const;

		//--------------------------------------------------------------------------------------------
		// 画面へ出す絵ができているか
		//
		// 画面に出るカメラに描画構成が設定されていて、組み上がっているときだけ true。
		// カメラが描画構成を持たなければ何も描かれない
		//--------------------------------------------------------------------------------------------
		bool IsPipelinePresentActive() const;

		// 画面へ出す絵。組み上がっていなければ nullptr
		const Resource::Texture* GetPresentTexture() const;

		// 新経路でパスが出力先として使うリソース名。
		// この名前で出力スロットを宣言したパスが、カメラの最終出力へ描くことになる
		static constexpr const char* kCameraOutputName = "CameraOutput";

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

		//--------------------------------------------------------------------------------------------
		// カメラ発の画面効果(被写界深度 / ラジアルブラー / 魚眼レンズ)
		//
		// どれもカメラの持ち物で、アクティブカメラのコンポーネントから
		// CamSetShaderSystem が毎フレーム詰める。速度に応じて動く値がここに乗る。
		//
		// パス側はアセットに保存した自分の値を既定として持っているので、
		// 「今フレーム、カメラから送られてきたか」を Is～Override() で見て、
		// 送られていればそちらを優先する。
		// 送られなかったフレームは EndFrame でフラグが落ちるので、
		// パスは自分の値へ戻る(効果が前フレームの値で固まらない)
		//--------------------------------------------------------------------------------------------
		void SetDoFData(const DoFOptionCB& a_data);
		const DoFOptionCB& GetDoFData() const;
		bool IsDoFOverride() const { return m_isDoFOverride; }

		void SetRadialBlurData(const RadialBlurOptionCB& a_data);
		const RadialBlurOptionCB& GetRadialBlurData() const;
		bool IsRadialBlurOverride() const { return m_isRadialBlurOverride; }

		void SetFishEyeData(const FishEyeOptionCB& a_data);
		const FishEyeOptionCB& GetFishEyeData() const;
		bool IsFishEyeOverride() const { return m_isFishEyeOverride; }
		// 環境データ
		void SetAmbientData(const AmbientData& a_data);
		const AmbientData& GetAmbientData() const;
		AmbientData& RefAmbientData();

		// スカイの設定とテクスチャ
		//
		// どちらもシーンに置いた SceneAmbientObject の持ち物で、毎フレーム流し込まれる。
		// テクスチャは所有せずハンドルだけ預かるので、置いた側が消えたら
		// 空のハンドルへ戻してもらう(空の間はスカイパスが何も描かない)。
		void SetSkyData(const SkyData& a_data);
		const SkyData& GetSkyData() const;
		SkyData& RefSkyData();

		void SetSkyTexture(const Handle<Resource::Texture>& a_handle);
		const Handle<Resource::Texture>& GetSkyTexture() const;

		// ライト
		//
		// ライトの実体はここのプールに置き、持ち主(シーンのオブジェクトなど)はハンドルだけ持つ。
		// GetFrameLightData() が返すのは今フレームぶんの GPU バッファで、
		// Execute() の中で詰め直されるのでレンダーパスからのみ引くこと。
		LightManager* RefLightManager();
		const FrameLightData& GetFrameLightData() const;
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
		/// <param name="a_emissiveScale">エミッシブスケール(エミッシブテクスチャに掛ける倍率)</param>
		/// <param name="a_emissiveAdd">マテリアルに依らない自己発光(加算・1.0超え可)</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Color& a_albedoScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = {1,1,1},
			const DXSM::Vector3& a_emissiveAdd = {0,0,0}
		);
		/// <summary>
		/// 指定したモデルを指定の座標に描画する命令 : 即時実行ではなく、コマンドとしてためたのちに一括で実行される
		/// </summary>
		/// <param name="a_world">ワールド</param>
		/// <param name="a_pModel">モデルのポインタ</param>
		/// <param name="a_worldMatrix">ワールド行列</param>
		/// <param name="a_prevMatrix">過去ワールド行列</param>
		/// <param name="a_albedoScale">カラースケール</param>
		/// <param name="a_emissiveScale">エミッシブスケール(エミッシブテクスチャに掛ける倍率)</param>
		/// <param name="a_emissiveAdd">マテリアルに依らない自己発光(加算・1.0超え可)</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Matrix& a_prevMatrix,
			const DXSM::Color& a_albedoScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = { 1,1,1 },
			const DXSM::Vector3& a_emissiveAdd = { 0,0,0 }
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
		/// <param name="a_emissiveScale">エミッシブスケール(エミッシブテクスチャに掛ける倍率)</param>
		/// <param name="a_emissiveAdd">マテリアルに依らない自己発光(加算・1.0超え可)</param>
		void SubmitModel(
			ECS::World& a_world,
			const Resource::Model* a_pModel,
			const DXSM::Matrix& a_worldMatrix,
			const DXSM::Matrix& a_prevMatrix,
			const RangeHandle<Resource::BoneMatrix>& a_boneHandle,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodePoseHandle,
			const Handle<Raytracing::DynamicRaytracingData>& a_animData,
			const DXSM::Color& a_albedoScale = Color::WHITE,
			const DXSM::Vector3& a_emissiveScale = { 1,1,1 },
			const DXSM::Vector3& a_emissiveAdd = { 0,0,0 }
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
			const Engine::Handle<Resource::NodePoseMatrix> nodePoseHnandle,
			const DXSM::Vector3& a_emissiveAdd = { 0,0,0 }	// 自己発光(加算)
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
		/// <param name="a_uvScale">
		/// UVに掛ける倍率(uv * uvScale + uvOffset)。既定は等倍。
		/// 1枚に並べた絵から1コマだけ出すときに、倍率でコマの大きさを指定する
		/// </param>
		/// <param name="a_curveK">
		/// 湾曲の強さ(1/px)。0で曲げない。
		/// 弧の中心から横へ dx(px) 離れた点が k*dx^2 だけ下がる。
		/// 開き角などの作り手が触る値からの変換は Decoration::Resolve が持つ
		/// </param>
		/// <param name="a_curveOffsetX">
		/// 弧の中心から、このクアッドの中心までの横ずれ(px)。
		/// 1つのUIが枠・中身・文字と複数のクアッドに分かれても、
		/// これを正しく渡せば全部が同じ1本の弧に乗る
		/// </param>
		void SubmitUI(
			const Handle<Resource::Texture>& a_texHandle,
			const Math::Vector2& a_pixelPos,
			const Math::Vector2& a_pixelSize,
			const Math::Color& a_color = {},
			float a_rotationDeg = 0,
			float a_layer = 0,
			const Math::Vector2& a_uvOffset = {},
			const Math::Vector2& a_pivot = { 0.5f, 0.5f },
			const Math::Vector2& a_uvScale = { 1.0f, 1.0f },
			float a_curveK = 0.0f,
			float a_curveOffsetX = 0.0f
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
		/// <param name="a_curveK">
		/// 湾曲の強さ(1/px)。0で曲げない。
		/// 弧の中心から横へ dx(px) 離れた点が k*dx^2 だけ下がる。
		/// 開き角などの作り手が触る値からの変換は Decoration::Resolve が持つ
		/// </param>
		/// <param name="a_curveOffsetX">
		/// 弧の中心から、このクアッドの中心までの横ずれ(px)。
		/// 1つのUIが枠・中身・文字と複数のクアッドに分かれても、
		/// これを正しく渡せば全部が同じ1本の弧に乗る
		/// </param>
		void SubmitUI(
			const Handle<Resource::Texture>& a_texHandle,
			const Math::Vector2& a_pixelPos,
			float a_scale = 1.0f,
			const Math::Color& a_color = Math::Color::White(),
			float a_rotationDeg = 0,
			float a_layer = 0,
			const Math::Vector2& a_uvOffset = {},
			const Math::Vector2& a_pivot = { 0.5f, 0.5f },
			float a_curveK = 0.0f,
			float a_curveOffsetX = 0.0f
		);

		// 追加
		UINT SetInstanceData(const MeshInstanceData& a_instanceData);
		UINT SetMeshMaterialData(const MeshMaterial& a_subsetData);
		void AddItem(const LightWeightDrawItem& a_item);

		// 取得
		std::span<const LightWeightDrawItem> GetPassItems(uint8_t a_passIndex);

		// パスの描画実行
		void BindPSO(Graphics::RenderContext* a_pCtx, uint8_t a_psoIndex);
		void BindPSO(Graphics::RenderContext* a_pCtx, const Handle<ID3D12PipelineState>& a_handle);
		// 配列取得
		const std::vector<SkinningDispatchItem>& GetSkinningImtes() const { return m_skinningDispathItemVec; }

		// バッファ取得
		MeshBufferAllocator* RefMeshBufferAllocator() { return m_upMeshBufferAllocator.get(); }

		const std::vector<UIData>& GetUIDataBuffer() { return m_uiDrawItemVec; }

		//--------------------------------------------------------------------------------------------
		// 描画用の板ポリ
		//
		// UIもパーティクルも同じ板ポリを使い回すので、フレームごとのレンダーコンテキストではなく
		// エンジンが1つずつ持つ(以前はコンテキストの数だけ同じ頂点バッファを作っていた)。
		//
		//   フラット … 4頂点の1枚板。曲げないものはすべてこれ
		//   湾曲用   … 横に kCurveDivision 分割した板。頂点が無いと曲げようがないので、
		//               UIの湾曲(UIData::IsCurved)が有効なものだけこちらで描く
		//--------------------------------------------------------------------------------------------

		// UIの湾曲用板ポリの横分割数
		static constexpr uint32_t kCurveDivision = 32;

		Resource::QuadPolygon* RefQuadPolygon()			{ return m_upQuadPolygon.get(); }
		Resource::QuadPolygon* RefCurvedQuadPolygon()	{ return m_upCurvedQuadPolygon.get(); }

	private:

		// このワールドのボーン行列をボーンパレットへ積み、GPU上の開始位置を返す。
		// 同じフレームで同じワールドを二度呼んでも積み直さず、最初に積んだ位置を返す
		uint32_t AcquireBoneBaseIndex(ECS::World& a_world);

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
			const Resource::Material*& a_pOutMaterial);

		// マテリアルとスケールからメッシュシェーダー用マテリアルデータを構築する。
		MeshMaterial BuildMeshMaterial(
			const Resource::Material* a_pMaterial,
			const DXSM::Color& a_albedoScale,
			const DXSM::Vector3& a_emissiveScale,
			const DXSM::Vector3& a_emissiveAdd);

		// 1つの描画コマンドを、シェーディングモデルが持つ全パスへ登録する共通処理。
		// (メッシュシェーダー用データ構築・PSO要求・描画アイテム登録をまとめて行う)
		void RegisterDrawCommandToPasses(
			const Resource::ModelDrawCommand& a_cmd,
			const Resource::Mesh* a_pMesh,
			const Resource::Material* a_pMaterial,
			const DXSM::Matrix& a_mat,
			const DXSM::Matrix& a_prevMat,
			bool a_isAnimation,
			uint32_t a_animatedVertexStart,
			const DXSM::Color& a_albedoScale,
			const DXSM::Vector3& a_emissiveScale,
			const DXSM::Vector3& a_emissiveAdd,
			PSOKey a_psoKey);

		//--------------------------------------------------------------------------------------------
		// カメラごとのパイプライン(新レンダーグラフ)
		//--------------------------------------------------------------------------------------------
		// 積まれたカメラの実行インスタンスを用意して回す
		void ExecuteCameraPipelines();

		// 積まれなかったカメラを捨てる(フレームの終わり)
		void PruneCameraPipelines();

		// メインカメラのパイプラインが描いた絵をバックバッファへ写す
		void PresentFromPipeline(D3D12::GraphicsCommandList* a_pCmdList);

		// 新パイプラインのモデル描画パスへパス番号を配り直す。
		// 取りっぱなしにすると組み直しのたびに番号が枯れるので、
		// どこか1つでも組み直したら全カメラぶんをまとめて配る
		void AssignPipelinePassIndices();

		//--------------------------------------------------------------------------------------------
		// 新パイプラインの、モデルを受け取るパスの一覧
		//
		// 描画アイテムはサブセット1つごとにパスの数だけ積むので、この一覧は
		// 1フレームに何万回も引かれる。毎回集め直すと submit がそれだけで重くなるため、
		// フレームの頭で1回作って引くだけにする
		//--------------------------------------------------------------------------------------------
		void RefreshPipelineGeometryPassCache();
		const std::vector<Pipeline::Pass*>& GetPipelineGeometryPasses(EGeometryQueue a_queue) const;

		// ピクセル空間で回転・アスペクト補正・ピボットを解決し、UIData(NDC基底)を
		// 1件バッファへ積む(SubmitUI 各オーバーロード共通)。
		void PushUIData(
			uint32_t a_texIndex,
			const Math::Vector2& a_pixelPos,
			const Math::Vector2& a_pixelSize,
			const Math::Color& a_color,
			float a_rotationDeg,
			float a_layer,
			const Math::Vector2& a_uvOffset,
			const Math::Vector2& a_pivot,
			const Math::Vector2& a_uvScale = { 1.0f, 1.0f },
			float a_curveK = 0.0f,
			float a_curveOffsetX = 0.0f
		);
	private:
		//--------------------------------------------------------------------------------------------
		// 主要クラス
		//--------------------------------------------------------------------------------------------
		// レンダーコンテキスト : 一フレーム内の描画情報を扱う
		std::vector<std::unique_ptr<RenderContext>> m_upRenderContextVec = {};
		UINT m_currentFrameIndex = 0;

		// PSOやルートシグネチャの管理
		D3D12::PipelineStateManager* m_pPipelineStateManager = nullptr;

		//メッシュバッファ管理
		std::unique_ptr<MeshBufferAllocator> m_upMeshBufferAllocator = nullptr;

		// 生成できるパスの型情報。インスタンスは持たない。
		// パイプラインアセットのロード時に、型IDからパスを作り直すのに使う
		std::unique_ptr<Pipeline::PassMetaRegistry> m_upPassMetaRegistry = nullptr;

		// 描画用の板ポリ(UI・パーティクル共用)。
		// フラットは4頂点の1枚板、湾曲用は横に kCurveDivision 分割したもの
		std::unique_ptr<Resource::QuadPolygon> m_upQuadPolygon = nullptr;
		std::unique_ptr<Resource::QuadPolygon> m_upCurvedQuadPolygon = nullptr;
	
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

		// スカイの設定と、引くスカイテクスチャ(所有はしない)
		SkyData m_cbSky = {};
		Handle<Resource::Texture> m_skyTexHandle = {};

		// 被写界深度データ(アクティブカメラの FocusParamComponent から毎フレーム設定)
		DoFOptionCB m_cbDoF = {};

		// 今フレーム、カメラから画面効果の値が送られてきたか。
		// EndFrame で落とすので、送られなかったフレームはパス側の値が使われる
		bool m_isDoFOverride = false;
		bool m_isRadialBlurOverride = false;
		bool m_isFishEyeOverride = false;

		// ラジアルブラーデータ(アクティブカメラの RadialBlurComponent から毎フレーム設定)
		RadialBlurOptionCB m_cbRadialBlur = {};

		// 魚眼レンズデータ(アクティブカメラの FishEyeComponent から毎フレーム設定)
		FishEyeOptionCB m_cbFishEye = {};

		// ライト本体のプール
		LightManager m_lightManager = {};

		// GPUへ渡すライト配列。
		// UPLOADヒープへ直接書き込むので、GPUがまだ前フレームを読んでいる領域を
		// 上書きしないようフレームぶん持つ(レンダーコンテキストと同じ数)
		FrameLightData m_frameLightDataArr[CPU_FRAME_COUNT] = {};
		
		// オブジェクト単位データ
		std::vector<MeshInstanceData> m_meshInstanceDataVec = {};

		// サブセット単位データ
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

		//--------------------------------------------------------------------------------------------
		// ボーンパレット
		//--------------------------------------------------------------------------------------------
		// ボーン行列はシーン(ワールド)ごとのプールに入っていて、添字も 0 から振り直される。
		// ポーズ画面のようにシーンを重ねて描くときは複数のワールドを1フレームで描くので、
		// ここで全ワールド分を1本に連結し、各ワールドの土台(開始位置)を覚えておく。
		std::vector<Resource::BoneMatrix> m_boneMatrixVec = {};
		std::unordered_map<const ECS::World*, uint32_t> m_boneBaseIndexMap = {};


		//--------------------------------------------------------------------------------------------
		// カメラ1台ぶんの描画データ
		//
		// パイプラインアセット(設計図)はリソースマネージャーが持ち、カメラはハンドルで参照する。
		// 実行用のグラフ(upRenderGraph)はカメラごとに1つ作る :
		// 設計図をそのまま回すと、同じアセットを指した2台が GBuffer も定数バッファも
		// 取り合って壊れるため。RenderGraph::BuildFrom() で設計図から複製する。
		//
		// 最終出力はどのカメラも自前のテクスチャへ描き、
		// メインカメラのものだけを最後にバックバッファへコピーする。
		// こうしておくとパス側は「メインかどうか」を知らなくてよく、
		// 同じアセットをメインにもモニターにも使い回せる
		//--------------------------------------------------------------------------------------------
		struct CameraPipelineData
		{
			// unique_ptr の中身(GraphicsPipeline)がこのヘッダーでは不完全型なので、
			// 生成と破棄は GraphicEngine.cpp 側(完全型が見える場所)に置く
			CameraPipelineData();
			~CameraPipelineData();

			// このカメラを識別する鍵
			const ECS::World* pWorld = nullptr;
			uint32_t entity = 0;

			CameraData cpuData = {};
			CameraData gpuData = {};

			// 描画用の配列

			// 使用するパイプラインのハンドル(設計図・所有しない)
			Handle<Pipeline::RenderingPipelineAsset> pipelineHandle = {};

			// このカメラ専用の実行インスタンス。
			// pipelineHandle の中身が差し替わったら作り直す
			std::unique_ptr<Pipeline::GraphicsPipeline> upPipeline = nullptr;

			// このカメラの最終出力
			std::unique_ptr<Resource::Texture> upFinalTex = nullptr;

			// 組み直しの判定用。
			// 設計図をエディターで触ると版が上がるので、そのときだけ作り直す
			uint32_t builtStructureVersion = 0;

			// 組めなかったことを知らせた版。
			// 失敗すると毎フレーム組み直しに来るので、同じ版で何度も言わないための印
			uint32_t reportedFailVersion = 0;
			uint32_t builtParamVersion = 0;
			UINT builtWidth = 0;
			UINT builtHeight = 0;

			// 今フレーム積まれたか。積まれなかったものは捨てる
			bool isSubmitted = false;

			// 描画する順番 : モニターに映すカメラを先に回してから本編を描く、といった並べ替え用
			int order = 0;
			bool isMain = false;
		};

		// 描画するパイプラインたち。
		// unique_ptr で持つのは、m_sortedCameras と m_pMainCamera が実体を指しているため。
		// 値のまま vector に入れると、カメラを1台足しただけで再確保が起きて
		// 保持しているポインタが全部ダングリングする
		std::vector<std::unique_ptr<CameraPipelineData>> m_cameras = {};

		// 使う順番などによりカメラをソートした配列
		std::vector<CameraPipelineData*> m_sortedCameras = {};

		// バックバッファに描画するもの
		CameraPipelineData* m_pMainCamera = nullptr;

		// モデルを受け取るパスの一覧(フレームの頭で作り直す)。
		// 実体はカメラのグラフが持っているので、ここは参照を並べるだけ
		std::vector<Pipeline::Pass*> m_pipelineOpaquePassVec = {};
		std::vector<Pipeline::Pass*> m_pipelineTransparentPassVec = {};

		// 直近にメインだったカメラの描画構成。
		// カメラが1台も積まれないフレーム(ゲームを止めているとき)でも、
		// 最後に画面を作っていた構成を借りられるように残しておく
		Handle<Pipeline::RenderingPipelineAsset> m_lastMainPipelineHandle = {};

		// 画面へ出す絵を新パイプラインから取るか(移植中の見比べ用)

		// 従来のレンダーグラフを回さないといけないか(エフェクトエディターが見ている間)

		// 新パイプラインのパスへ配るパス番号。
		// 255 から下って使う(従来経路は 0 から上っていく)
	};
}