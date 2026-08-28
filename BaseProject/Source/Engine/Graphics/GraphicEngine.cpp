#include "GraphicEngine.h"

#include "../MainEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../D3D12/PipelineStateManager/PipelineStateManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "RenderGraph/RenderGraph.h"
#include "../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../Particle/ParticleBufferManager.h"
#include "RenderPassRegistry/RenderPassRegistry.h"
#include "MeshBufferAllocator/MeshBufferAllocator.h"
#include "MouseCursor/MouseCursor.h"

// シーン
#include "../Scene/BaseScene/BaseScene.h"
#include "../Scene/SceneManager/SceneManager.h"

// ECS
#include "../ECS/World/World.h"

// オプション
#include "../Option/OptionManager.h"

// レンダーパス
#include "RenderPass/Geometry/ZPrePass/ZPrePass.h"
#include "RenderPass/Geometry/GBufferPass/GBufferPass.h"
#include "RenderPass/Geometry/DebugLinePass/DebugLinePass.h"
#include "RenderPass/Geometry/ParticlePass/ParticlePass.h"
#include "RenderPass/Geometry/FullRaytracingPass/FullRaytracingPass.h"

#include "RenderPass/Lighting/DeferredLighting/DeferredLighting.h"
#include "RenderPass/Lighting/RaytracingGIPass/RaytracingGIPass.h"

#include "RenderPass/Lighting/Shadow/RaytracingShadowPass/RaytracingShadowPass.h"

#include "RenderPass/Sky/SkyPass/SkyPass.h"

#include "RenderPass/PostEffect/AntiAliasing/TAA/TAAPass.h"
#include "RenderPass/PostEffect/DoF/CoCPass/CoCPass.h"
#include "RenderPass/PostEffect/DoF/DoFPass/DoFPass.h"
#include "RenderPass/PostEffect/Blur/GaussianBlurPass/GaussianBlurPass.h"
#include "RenderPass/PostEffect/Blur/RadialBlurPass/RadialBlurPass.h"
#include "RenderPass/PostEffect/Bloom/BloomExtractPass/BloomExtractPass.h"
#include "RenderPass/PostEffect/Bloom/KawaseBlurPass/KawaseBlurPass.h"
#include "RenderPass/PostEffect/Bloom/BloomCompositePass/BloomCompositePass.h"
#include "RenderPass/PostEffect/Denoise/GI/GISpatialDenoisePass/GISpatialDenoisePass.h"
#include "RenderPass/PostEffect/Denoise/GI/GITempralAccumulationPass/GITemporalAccumulationPass.h"
#include "RenderPass/PostEffect/Denoise/Shadow/ShadowSpatialDenoisePass/ShadowSpatialDenoisePass.h"
#include "RenderPass/PostEffect/Denoise/Shadow/ShadowTemporalAccumulationPass/ShadowTemporalAccumulationPass.h"
#include "RenderPass/PostEffect/ToneMap/ToneMapPass.h"
#include "RenderPass/Present/CopyToBackBufferPass/CopyToBackBufferPass.h"

#include "RenderPass/Particle/UpdateParticlePass/UpdateParticlePass.h"
#include "RenderPass/Particle/EmitParticlePass/EmitParticlePass.h"

#include "RenderPass/Skinning/SkinningPass.h"
#include "RenderPass/Skinning/UpdateBLASPass/UpdateBLASPass.h"

#include "RenderPass/Utility/GBufferHistoryPass/GBufferHistoryPass.h"
#include "RenderPass/Utility/PostHistoryPass/PostHistoryPass.h"

#include "RenderPass/UpScale/FullRaytracingUpScalePass/FullRaytracingUpScalePass.h"

#include "RenderPass/UI/UIPass/UIPass.h"


// テスト
#include "../../Application/Game/GameManager/GameManager.h"

namespace Engine::Graphics
{
	namespace
	{
		// vector を空にしつつ、次フレームぶんの容量を確保し直す(EndFrame用)。
		template<typename T>
		void ClearAndReserve(std::vector<T>& a_vec, size_t a_reserveCount)
		{
			a_vec.clear();
			a_vec.reserve(a_reserveCount);
		}
	}

	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(
		D3D12::GraphicsCommandList* a_pCmdList,
		const GraphicsEngineDesc& a_desc
	)
	{

		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

		m_pPipelineStateManager = a_desc.pPipelineStateManager;

		// レンダーコンテキストの作成
		for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			auto _upCtx = std::make_unique<RenderContext>();

			RenderContextDesc _desc = {};
			_desc.pDevice = _pDevice;

			_desc.cbAllocatorMemSize = 32 * 1024 * 1024;
			// シーンを重ねて描くとき(ポーズ画面など)は全ワールドのボーン行列を
			// 1本のパレットへ連結するので、1ワールド分(プールの確保数)では足りない
			_desc.boneElementNum = 40000;

			_upCtx->Init(this, a_pCmdList, _desc);
			m_upRenderContextVec.push_back(std::move(_upCtx));
		}

		// ライト
		// バッファは上限ぶんを固定確保する(FrameLightData::Create の中)。
		// ライトが増えるたびに作り直すと、GPUが読んでいる最中のリソースを解放することになる
		m_lightManager.Init();
		for (auto& _frameLight : m_frameLightDataArr)
		{
			_frameLight.Create(_pDevice);
		}

		// レンダーパスの登録
		m_upRenderPassRegistry = std::make_unique<RenderPassRegistry>();
		// ラスター関係


		AddSkinningPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Setup);
		AddUpdateBLASPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Setup);

		AddZPrePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Setup);
		AddGBufferPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Geometry);
		AddDebugLinePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::UI);
		// 最終段 : HDR をトーンマップして FinalColor を作り、それをバックバッファへ載せる。
		// どちらも Present 帯。FinalColor の書き手→読み手の関係でグラフがこの順に並べる
		AddToneMapPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Present);
		AddCopyToBackBufferPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Present);
		//AddFullRaytracingPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Geometry);

		// 影レイトレは GBuffer の深度と法線からピクセル位置・裏面判定を復元するため、
		// 必ず GBufferPass(Geometry) より後で実行する必要がある。
		// Shadow フェーズ(=1) は Geometry フェーズ(=2) より前に走るため、以前は
		// 「今フレームの深度 + 前フレームの法線」で影を計算してしまい、カメラ移動時に
		// 輪郭で裏面カリング/バイアスが誤爆して黒いゴーストが出ていた。
		// GI と同じ Raytracing フェーズ(=3, Geometry の後)へ移動して現在フレームの法線を読む。
		AddRaytracingShadowPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Raytracing);
		AddRaytracingGIPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Raytracing);
		AddDeferredLighting(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Lighting);
		// スカイはライティングの結果(AfterLighting)へ直接描くので、必ずディファードライティングより後。
		// 同一フェーズ内はトポロジカルソートで並ぶが、この2つの間には
		// 「読む→書く」の関係が無い(スカイは上書きするだけ)ので辺が張られない。
		// 辺が無いものは登録順で並ぶため、ここの順番がそのまま実行順になる
		AddSkyPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Lighting);
		AddGBufferHistoryPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::HistoryUpdate);
		AddPostHistoryPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::HistoryUpdate);

		AddUIPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::UI);

		AddTAAPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);

		// 被写界深度。ボカした絵にTAAを掛けると履歴がにじむので、必ずTAAの後に登録する。
		// (同一フェーズ内はリソースのバージョンで依存が決まるため、登録順が
		//  「TAAの出力を読む」という関係の解決に効く)
		AddCoCPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);
		AddDoFPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);

		// ------------------------------------------------------------------
		// 川瀬式ブルーム
		//
		//   抽出(等倍) → 1/2 → 1/4 → 1/8 → 1/16 と縮小しながらガウシアンブラー
		//              → 4枚を平均して1枚に → メインカラーへ加算
		//
		// 縮小率ごとにボケの広がりが変わるので、それを重ねると
		// 「芯は明るく、外へ行くほどゆるく広がる」ブルーム特有の減衰になる。
		// 同じ広がりを1回の大きなブラーで出そうとするとタップ数が跳ね上がるため、
		// 縮小バッファを積むこの形が安い。
		//
		// 等倍へ戻す拡大パスは持たない。合流(KawaseBlurPass)がUVでサンプリングするので、
		// 解像度の違いはサンプラーのバイリニアが吸収してくれる。
		//
		// 合成はメインカラー(AfterTAAColor)を読んで書き戻すので、必ずDoFより後に登録する。
		// (同一フェーズ内はリソースのバージョンで依存が決まるため、登録順が
		//  「DoFの出力を読む」という関係の解決に効く)
		// ------------------------------------------------------------------
		{
			// 高輝度成分の抽出(等倍)
			AddBloomExtractPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);

			// 各段の解像度スケール
			constexpr float kBloomScales[4] = { 0.5f, 0.25f, 0.125f, 0.0625f };

			// ブラーの広がり。すべて縮小後の低解像度で回るので広め(5x5)に取れる
			constexpr float kBlurSigma = 1.2f;
			constexpr int   kBlurTapRadius = 2;

			const std::string _extractName = "BloomExtract";

			// 縮小 : 1つ前の段を入力にして半分ずつ小さくしていく
			for (int _i = 0; _i < 4; ++_i)
			{
				const float _srcScale = (_i == 0) ? 1.0f : kBloomScales[_i - 1];
				const std::string _srcName = (_i == 0) ? _extractName : ("BloomBlurDown" + std::to_string(_i - 1));

				AddGaussianBlurPass(
					m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess,
					"BloomBlurDownPass" + std::to_string(_i),
					_srcName,
					"BloomBlurDown" + std::to_string(_i),
					_srcScale, kBloomScales[_i],
					kBlurSigma, kBlurTapRadius
				);
			}

			// 4枚を1枚のブルームへまとめる（拡大はここのサンプリングが兼ねる）
			AddKawaseBlurPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);

			// メインカラーへ加算合成して固定名へ戻す
			AddBloomCompositePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);
		}

		// ラジアルブラー。ブルームの後に登録すること。
		// 光ったところごと放射状に流れてほしいので、逆にすると
		// 引きずった跡だけが後から光ってしまう。
		// (同一フェーズ内はリソースのバージョンで依存が決まるため、登録順が
		//  「ブルーム合成の出力を読む」という関係の解決に効く)
		AddRadialBlurPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::PostProcess);

		AddShadowTemporalAccumulationPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);
		// 影はテンポラルのみだと履歴依存が強くゴーストが出るため、蓄積後にスペースデノイズをかける。
		// NotSort は登録順で実行されるので、必ずテンポラルの後に登録すること。
		AddShadowSpatialDenoisePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);

		// テンポラルデノイズ前に生のRayGIへ一度スペースデノイズをかける(プリデノイズ)。
		// NotSort は登録順で実行されるため、必ずテンポラルパスより前に登録すること。
		// RayGI は R16G16B16A16_FLOAT(HDR) なので、出力も同フォーマットにしてレンジを潰さない。
		AddGISpatialDenoisePass(
			m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort,
			"GIPreSpatialDenoisePass", "RayGI", "RayGIDenoised", 2, DXGI_FORMAT_R16G16B16A16_FLOAT);

		AddGITemporalAccumulationPass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);
		// GIは最後までHDRを保つため、スペースデノイズの出力(FinalGI)/中間バッファもR16Fにする
		AddGISpatialDenoisePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort,
			"GISpatialDenoisePass", "DenoiseGI", "FinalGI", 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
		AddFullRaytracingUpScalePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::NotSort);
		// パーティクル
		AddEmitParticlePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Particle);
		AddUpdateParticlePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Particle);
		AddParticlePass(m_pPipelineStateManager, m_upRenderPassRegistry.get(), Graphics::EDrawPhase::Particle);

		// レンダーグラフの構築
		m_upRenderGraph = std::make_unique<RenderGraph>();
		m_upRenderGraph->Init(m_upRenderPassRegistry.get());

		// 定数バッファ初期化
		m_cbAmbient = {};
		m_cbAmbient.ambientColorScale = { 0,0,0 };

		// バッファ管理クラス
		BufferSizeDesc _bufferSizeDesc = {};
		_bufferSizeDesc.staticVertexBufferSize = 20000000;
		_bufferSizeDesc.indexBufferSize = 20000000;
		_bufferSizeDesc.animatedVertexBufferSize = 5000000;
		m_upMeshBufferAllocator = std::make_unique<MeshBufferAllocator>();
		m_upMeshBufferAllocator->Init(
			_pDevice,
			a_pCmdList,
			_bufferSizeDesc
		);

	}

	void GraphicsEngine::Release()
	{
		// レンダーコンテキスト解放
		for (auto& _ctx : m_upRenderContextVec)
		{
			_ctx->Release();
			_ctx.reset();
		}

		// ライト解放
		// プールを空にした時点で配り済みのライトハンドルはすべて無効になる
		for (auto& _frameLight : m_frameLightDataArr)
		{
			_frameLight.Release();
		}
		m_lightManager.Release();

		// レンダーグラフの一時リソース(GBuffer/TAA/各種RTなどのテクスチャ・バッファ)を解放。
		// デストラクタ任せにすると LIVE_DEVICE として残るため、ここで明示的に解放する。
		// DescriptorHeapManager の解放より前に呼ぶ必要がある。
		if (m_upRenderGraph)
		{
			m_upRenderGraph->Release();
		}

		// 各リンク解除
		m_pPipelineStateManager = nullptr;


		m_upMeshBufferAllocator->Release();

	}

	void GraphicsEngine::BeginFrame()
	{
		// 今から使うレンダーコンテキスをクリア
		m_currentFrameIndex = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();
		m_upRenderContextVec[m_currentFrameIndex]->Clear();
	}
	void GraphicsEngine::Execute()
	{
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();
		// GPUが実際に完了させた値 : これ以下でタグ付けされた領域だけをフリーリストに戻す
		auto _completedFence = D3D12::D3D12Wrapper::Instance().GetCompletedFenceValue();

		// レイトレ用BLAS初期化 : 初期化命令があれば走る
		ProcessInitQueue(_pDevice, _pCmdList);

		// テスト
		App::Game::GameManager::Instance().Draw();

		// 自前のマウスカーソルを最前面へ。
		// UIパスは深度を切ってあるので積んだ順がそのまま前後になる。
		// シーンのUIを全部積み終えたここで積むことで、必ず一番手前に出る。
		//
		// ゲームモード以外はエディターのImGuiが上に重なるので、あちらの
		// 最前面レイヤーへ描く(MouseCursor::DrawImGui)。ここでは積まない
		if (MainEngine::Instance().GetMode() == EAppMode::Game)
		{
			if (auto* _pCursor = MainEngine::Instance().RefMouseCursor())
			{
				_pCursor->SubmitUI(this);
			}
		}

		// メッシュバッファの更新
		m_upMeshBufferAllocator->UpdateFrame(_pCmdList, _completedFence);

		// パーティクルのバッファ更新
		MainEngine::Instance().RefParticleManager()->UploadEmitData(_pCmdList);

		// バックバッファのresourceバリア
		D3D12::ResourceBarrier(
			_pCmdList, // D3D12Wrapper側のバリア関数も引数でリストをもらうように修正してください
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBufferTex().GetRTV()
		);
		float _clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f }; // 背景色
		_pCmdList->ClearRenderTargetView(_cpuHandle, _clearColor, 0, nullptr);

		// レンダーコンテキストにコマンドリストをセット
		m_upRenderContextVec[m_currentFrameIndex]->SetDirectCommandList(_pCmdList);

		// カメラの割り込み(エディターカメラなど)
		// ECS側のカメラ設定は上の GameManager::Draw() 内(PreDrawフェーズ)で行われるため、
		// 上書きするなら必ずこの位置(GPUデータ作成の直前)で行うこと。
		if (m_isCameraOverride)
		{
			SetCameraMat(m_cameraOverrideWorldMat);
			SetProjMat(m_cameraOverrideProjMat);
		}

		// GPU用カメラデータを作成
		CreateGPUCameraData();

		// バッファの更新
		// ボーン行列は上の GameManager::Draw() で描くワールドぶんだけ積まれている。
		// 「今の一番上のシーン」から引いてはいけない(ポーズ中は後ろのゲームのボーンが載らない)
		m_upRenderContextVec[m_currentFrameIndex]->UpdateBuffer(
			m_meshInstanceDataVec, m_meshMaterialDataVec,
			m_boneMatrixVec
		);
		//------------------------------------------------------------------
		// UIをレイヤー順に並べ替える
		//
		// UIパスは深度を持たない(DepthEnable(false))ので、重なりを決めるのは
		// 描く順そのもの。並べ替えないとレイヤーの値がどこにも効かない。
		//
		// 小さいものから描く = 大きいほど手前。
		// 同じ値のものは積んだ順を崩さないよう stable_sort を使う
		// (1つのUIが持つ飾りは配列順で重なっているため、崩すと絵が入れ替わる)
		//------------------------------------------------------------------
		std::stable_sort(
			m_uiDrawItemVec.begin(), m_uiDrawItemVec.end(),
			[](const UIData& a, const UIData& b)
			{
				return a.layer < b.layer;
			}
		);

		m_upRenderContextVec[m_currentFrameIndex]->UpdateUIBuffer(m_uiDrawItemVec);

		// ライトをGPUバッファへ詰め直す。
		// レンダーパスが引くのはこの結果なので、必ずレンダーグラフの実行より前に済ませる
		m_lightManager.BuildFrameData(m_frameLightDataArr[m_currentFrameIndex]);


		// 描画アイテムをソート
		std::sort(
			m_lightWeightDrawItemVec.begin(), m_lightWeightDrawItemVec.end(),
			[](const LightWeightDrawItem& a, const LightWeightDrawItem& b)
			{
				return a.sortKey.value < b.sortKey.value;
			}
		);

		// レンダーパス実行
		m_upRenderGraph->Execute(this, m_upRenderContextVec[m_currentFrameIndex].get());

		D3D12::D3D12Wrapper::Instance().SubmitDirectCommandList(_pCmdList);
		m_upRenderContextVec[m_currentFrameIndex]->SetDirectCommandList(nullptr);

	}
	void GraphicsEngine::EndFrame()
	{
		// 描画命令をクリアしてメモリ領域を確保しておく
		ClearAndReserve(m_lightWeightDrawItemVec, 10000);
		ClearAndReserve(m_uiDrawItemVec, 10000);
		ClearAndReserve(m_dynamicRayRequestVec, 1000);
		ClearAndReserve(m_skinningDispathItemVec, 1000);

		// ボーンパレットと、ワールドごとの土台の対応表
		ClearAndReserve(m_boneMatrixVec, 10000);
		m_boneBaseIndexMap.clear();

		// オブジェクトデータの消去
		ClearAndReserve(m_meshInstanceDataVec, 10000);

		// サブセット情報の消去
		ClearAndReserve(m_meshMaterialDataVec, 10000);

		// 被写界深度は毎フレーム、アクティブカメラが設定し直す。
		// ここで落としておけば、カメラが居ない/ピント設定を持たないフレームは
		// 前フレームの値でボケ続けることなく素通しになる
		m_cbDoF = {};

		// ラジアルブラーも同じ。設定し直されなかったフレームは無効(流れない)
		m_cbRadialBlur = {};

		// デバッグ用配列のクリア
		Editor::MainEditor::Instance().ClearBuffer();
	}

	const Graphics::RenderContext* GraphicsEngine::GetRenderContext() const
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
	}
	Graphics::RenderContext* GraphicsEngine::RefRenderContext()
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();

	}
	D3D12::PipelineStateManager* GraphicsEngine::RefPipelineStateManager()
	{
		return m_pPipelineStateManager;
	}
	Graphics::RenderPassRegistry* GraphicsEngine::RefRenderPassRegistry()
	{
		return m_upRenderPassRegistry.get();
	}
	RenderGraph* GraphicsEngine::RefRenderGraph()
	{
		return m_upRenderGraph.get();
	}

	LightManager* GraphicsEngine::RefLightManager()
	{
		return &m_lightManager;
	}

	const FrameLightData& GraphicsEngine::GetFrameLightData() const
	{
		return m_frameLightDataArr[m_currentFrameIndex];
	}
	void GraphicsEngine::SetCameraMat(const DXSM::Matrix& a_worldMat)
	{
		// 座標を代入
		m_cbCamera.pos = { a_worldMat._41,a_worldMat._42,a_worldMat._43 ,1 };

		// ビュー行列・逆ビュー行列をセット
		m_cbCamera.viewMat = a_worldMat.Invert();
		m_cbCamera.viewInvMat = a_worldMat;
	}
	void GraphicsEngine::SetProjMat(const DXSM::Matrix& a_projMat)
	{
		m_cbCamera.projMat = a_projMat;
		m_cbCamera.projInvMat = a_projMat.Invert();
	}
	void GraphicsEngine::SetDoFData(const DoFOptionCB& a_data)
	{
		m_cbDoF = a_data;
	}
	const DoFOptionCB& GraphicsEngine::GetDoFData() const
	{
		return m_cbDoF;
	}
	void GraphicsEngine::SetRadialBlurData(const RadialBlurOptionCB& a_data)
	{
		m_cbRadialBlur = a_data;
	}
	const RadialBlurOptionCB& GraphicsEngine::GetRadialBlurData() const
	{
		return m_cbRadialBlur;
	}
	void GraphicsEngine::SetCameraOverride(const DXSM::Matrix& a_worldMat, const DXSM::Matrix& a_projMat)
	{
		m_isCameraOverride = true;
		m_cameraOverrideWorldMat = a_worldMat;
		m_cameraOverrideProjMat = a_projMat;
	}
	void GraphicsEngine::ClearCameraOverride()
	{
		m_isCameraOverride = false;
	}
	const CameraData& GraphicsEngine::GetCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetGPUCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetCPUCameraData() const
	{
		return m_cbCamera;
	}
	void GraphicsEngine::SetAmbientData(const AmbientData& a_data)
	{
		m_cbAmbient = a_data;
	}
	const AmbientData& GraphicsEngine::GetAmbientData() const
	{
		return m_cbAmbient;
	}
	AmbientData& GraphicsEngine::RefAmbientData()
	{
		return m_cbAmbient;
	}
	void GraphicsEngine::SetSkyData(const SkyData& a_data)
	{
		m_cbSky = a_data;
	}
	const SkyData& GraphicsEngine::GetSkyData() const
	{
		return m_cbSky;
	}
	SkyData& GraphicsEngine::RefSkyData()
	{
		return m_cbSky;
	}
	void GraphicsEngine::SetSkyTexture(const Handle<Resource::Texture>& a_handle)
	{
		m_skyTexHandle = a_handle;
	}
	const Handle<Resource::Texture>& GraphicsEngine::GetSkyTexture() const
	{
		return m_skyTexHandle;
	}
	//======================================================================================
	// ボーンパレットへこのワールドのボーン行列を積む
	//--------------------------------------------------------------------------------------
	// ボーン行列はワールド(シーン)ごとの RangePool に入っていて、その添字も
	// ワールドごとに 0 から始まる。GPU側のボーンパレットは1本しかないので、
	// 複数のシーンを重ねて描くときは連結したうえで土台を足してやる必要がある。
	//
	// ポーズ画面はゲームのシーンへ重ねて出す(Push)ので、描くワールドが2つになる。
	// ここで両方を積んでおかないと、片方のキャラのボーンが単位行列でも他人のものでもない
	// 場所を指し、頂点が一点に潰れて消えたように見える。
	//
	// 呼ばれるのは描画フェーズ(GameManager::Draw)の中で、ボーン行列自体は
	// それより前の Animation フェーズで確定しているので、この時点の値で正しい。
	//======================================================================================
	uint32_t GraphicsEngine::AcquireBoneBaseIndex(ECS::World& a_world)
	{
		// 同じワールドをこのフレームで既に積んでいればその位置を返す
		auto _it = m_boneBaseIndexMap.find(&a_world);
		if (_it != m_boneBaseIndexMap.end()) return _it->second;

		const uint32_t _baseIndex = static_cast<uint32_t>(m_boneMatrixVec.size());

		if (a_world.HasResource<Pool::RangePool<Resource::BoneMatrix>>())
		{
			auto& _boneMatPool = a_world.GetResource<Pool::RangePool<Resource::BoneMatrix>>();

			// プールは最初から10000要素ぶん確保されているので、丸ごと積むと
			// ワールドを2つ重ねただけでGPU側のボーンパレットが溢れる。
			// 実際に使われている末尾までで足りる(ハンドルの添字は必ずこの内側)
			const auto& _data = _boneMatPool.GetAllData();
			const size_t _usedCount = (std::min)(
				static_cast<size_t>(_boneMatPool.GetUsedCount()), _data.size());

			m_boneMatrixVec.insert(m_boneMatrixVec.end(), _data.begin(), _data.begin() + _usedCount);
		}

		m_boneBaseIndexMap.emplace(&a_world, _baseIndex);
		return _baseIndex;
	}

	void GraphicsEngine::SubmitSkinning(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const Handle<Raytracing::DynamicRaytracingData> dynamicHandle,
		const RangeHandle<Resource::NodePoseMatrix> nodePoseHnandle,
		const RangeHandle<Resource::BoneMatrix> boneHandle
	)
	{
		// このワールドのボーン行列をパレットへ積み、GPU上の土台を得る
		const uint32_t _boneBaseIndex = AcquireBoneBaseIndex(a_world);

		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();
		for (const auto& _cmd : _drawCmdVec)
		{
			// マテリアル取得
			auto* _pMaterial = Engine::Resource::ResourceManager::Instance().Get(_cmd.materialHandle);
			if (!_pMaterial) continue;

			// メッシュ取得
			auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_cmd.meshHandle);
			if (!_pMesh) continue;

			// レイトレ用データを持たないメッシュはスキニング登録できない
			if (!_pMesh->HasRtData()) continue;

			// マテリアルからシェーディングモデルを取得
			auto* _pShadingModel = Engine::Resource::ResourceManager::Instance().Get(_pMaterial->shadingModelHandle);
			if (!_pShadingModel) continue;

			SkinningDispatchItem _item = {};
			_item.pWorld = &a_world;
			_item.staticVertexHandle = _pMesh->GetRtData().vertexHandle;
			_item.staticIndexHandle = _pMesh->GetRtData().indexHandle;
			_item.nodePoseMat = nodePoseHnandle;
			_item.animHandle = dynamicHandle;
			_item.boneHandle = boneHandle;

			// スキニングのコンピュートが読むのはGPU上の位置なので土台を足す
			_item.boneBufferStart = _boneBaseIndex + boneHandle.startIndex;

			auto& _pool = a_world.GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();
			auto* _data = _pool.Get(dynamicHandle);
			if (!_data) continue;

			for (auto& _meshData : _data->meshDataVec)
			{
				if (_cmd.meshHandle == _meshData.meshHandle)
				{
					_item.animatedHandle = _meshData.animatedVertexHandle;
				}
			}

			m_skinningDispathItemVec.push_back(_item);
		}
	}
	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd
	)
	{
		SubmitModel(
			a_world,
			a_pModel,
			a_worldMatrix,
			a_worldMatrix,
			a_albedoScale,
			a_emissiveScale,
			a_emissiveAdd
		);
	}

	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Matrix& a_prevMatrix,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd
	)
	{
		if (!a_pModel) return;

		// モデルが持っている描画コマンド（サブセット）を展開
		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();

		for (const auto& _cmd : _drawCmdVec)
		{
			// -----------------------------------------------------
			// リソースの取得と検証
			// -----------------------------------------------------
			const Resource::Mesh* _pMesh = nullptr;
			const Resource::Material* _pMaterial = nullptr;
			const Resource::ShadingModelTable* _pShadingModel = nullptr;
			if (!FetchDrawResources(_cmd, _pMesh, _pMaterial, _pShadingModel)) continue;

			// -----------------------------------------------------
			// 行列計算
			// -----------------------------------------------------
			DXSM::Matrix _nodeTransMat(a_pModel->GetOriginalNodeVec()[_cmd.nodeIndex].worldTransform);
			DXSM::Matrix _mat = _nodeTransMat * a_worldMatrix;
			DXSM::Matrix _prevMat = _nodeTransMat * a_prevMatrix;

			// -----------------------------------------------------
			// PermutationFlags の構築
			// -----------------------------------------------------
			uint32_t _flags = (uint32_t)Engine::Graphics::EShaderPermutationFlags::None;

			bool _isAnimation = false; // ボーンがあるか等で判定
			_flags |= (uint32_t)(_isAnimation ?
				Engine::Graphics::EShaderPermutationFlags::Skinned :
				Engine::Graphics::EShaderPermutationFlags::Static);

			if (_cmd.alphaMode == Engine::Resource::Alpha::Mask) {
				_flags |= (uint32_t)Engine::Graphics::EShaderPermutationFlags::AlphaMasked;
			}

			Engine::Graphics::PSOKey _psoKey = {};
			_psoKey.shadingModelTableHandle = _pMaterial->shadingModelHandle;
			_psoKey.permutationFlags = _flags;

			// -----------------------------------------------------
			// 各パスへの描画アイテム登録(共通処理)
			// -----------------------------------------------------
			RegisterDrawCommandToPasses(
				_cmd, _pMesh, _pMaterial, _pShadingModel,
				_mat, _prevMat,
				_isAnimation, 0 /*animatedVertexStart*/,
				a_albedoScale, a_emissiveScale, a_emissiveAdd, _psoKey);
		}
	}

	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Matrix& a_prevMatrix,
		const RangeHandle<Resource::BoneMatrix>& a_boneHandle,
		const RangeHandle<Resource::NodePoseMatrix>& a_nodePoseHandle,
		const Handle<Raytracing::DynamicRaytracingData>& a_animData,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd
	)
	{
		// ノード行列取得
		auto& _nodePosePool = a_world.GetResource<Pool::RangePool<Resource::NodePoseMatrix>>();
		const auto& _nodePoseMatVec = _nodePosePool.GetRange(a_nodePoseHandle);

		// アニメーション後データ (※1つのモデルに対して共通ならループ外で取得・チェックすると効率的です)
		auto& _pool = a_world.GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();
		auto* _data = _pool.Get(a_animData);
		if (!_data) return;

		// このワールドのボーン行列をパレットへ積む。
		// 戻り値の土台位置はここでは使わない(頂点をスキニングするのはコンピュートの
		// スキニングパスで、描画側はその結果の頂点バッファを読むだけ)。
		// ただし積むこと自体はそのパスに要るので、呼び出しを外してはいけない
		AcquireBoneBaseIndex(a_world);

		// モデルが持っている描画コマンド（サブセット）を展開
		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();
		for (const auto& _cmd : _drawCmdVec)
		{
			// メッシュ・マテリアル・シェーディングモデルの取得と検証
			const Resource::Mesh* _pMesh = nullptr;
			const Resource::Material* _pMaterial = nullptr;
			const Resource::ShadingModelTable* _pShadingModel = nullptr;
			if (!FetchDrawResources(_cmd, _pMesh, _pMaterial, _pShadingModel)) continue;

			// -----------------------------------------------------
			// アニメーション用頂点オフセットの検索
			// -----------------------------------------------------
			uint32_t _animatedVertexStart = 0;
			if (_data)
			{
				for (const auto& _meshData : _data->meshDataVec)
				{
					if (_cmd.meshHandle == _meshData.meshHandle)
					{
						_animatedVertexStart = _meshData.animatedVertexHandle.startIndex;
						break; // 見つかったらループを抜ける
					}
				}
			}

			// ノードのワールド行列を確定
			// モデル差し替え直後などで描画コマンドとポーズ領域のサイズが食い違った場合は
			// クラッシュさせずこのコマンドの描画をスキップする
			if (_cmd.nodeIndex >= _nodePoseMatVec.size()) continue;
			DXSM::Matrix _nodeTransMat(_nodePoseMatVec[_cmd.nodeIndex].world);
			DXSM::Matrix _mat = _nodeTransMat * a_worldMatrix;

			DXSM::Matrix _prevMat = _nodeTransMat * a_prevMatrix;

			// =========================================================
			// PermutationFlags を構築
			// =========================================================
			uint32_t _flags = (uint32_t)Engine::Graphics::EShaderPermutationFlags::None;

			// アニメーション判定
			bool _isAnimation = (a_boneHandle.count > 0);
			_flags |= (uint32_t)(_isAnimation ?
				Engine::Graphics::EShaderPermutationFlags::Skinned :
				Engine::Graphics::EShaderPermutationFlags::Static);

			// アルファモード判定
			if (_cmd.alphaMode == Engine::Resource::Alpha::Mask) {
				_flags |= (uint32_t)Engine::Graphics::EShaderPermutationFlags::AlphaMasked;
			}

			// PSOKey作成
			Engine::Graphics::PSOKey _psoKey = {};
			_psoKey.shadingModelTableHandle = _pMaterial->shadingModelHandle;
			_psoKey.permutationFlags = _flags;

			// =========================================================
			// 各パスへ描画アイテムを投げる(共通処理)
			// =========================================================
			RegisterDrawCommandToPasses(
				_cmd, _pMesh, _pMaterial, _pShadingModel,
				_mat, _prevMat,
				_isAnimation, _animatedVertexStart,
				a_albedoScale, a_emissiveScale, a_emissiveAdd, _psoKey);
		}
	}

	void GraphicsEngine::SubmitModel(const DXSM::Matrix& a_worldMat, const DXSM::Vector4& a_colorScale, const DXSM::Vector3& a_emissiveScale, const Engine::Handle<Raytracing::DynamicRaytracingData> dynamicHandle, const Engine::Handle<Resource::NodePoseMatrix> nodePoseHnandle, const DXSM::Vector3& a_emissiveAdd)
	{

		m_dynamicRayRequestVec.push_back(
			{ a_worldMat,a_colorScale,a_emissiveScale,a_emissiveAdd,dynamicHandle,nodePoseHnandle }
		);
	}

	void GraphicsEngine::SubmitUI(const Handle<Resource::Texture>& a_texHandle, const Math::Vector2& a_screenPos, const Math::Vector2& a_screenRect, const Math::Color& a_color, float a_rotation, float a_layer, const Math::Vector2& a_uvOffset, const Math::Vector2& a_pivot, const Math::Vector2& a_uvScale, const Math::Vector2& a_curveCenter,
		float a_curveRadius,
		float a_curveAngle)
	{
		auto& _resMgr = Resource::ResourceManager::Instance();

		// 読み込みが終わっていないものは、そのフレームは描かない。
		// 非同期ロード中のスロットには空の実体が入っているため、
		// ポインタのnullチェックだけでは弾けない
		if (!_resMgr.IsReady(a_texHandle)) return;

		auto* _pTex = _resMgr.Get(a_texHandle);
		if (!_pTex) return;

		// サイズは呼び出し側の指定値をそのまま使う
		PushUIData(_pTex->GetSRV().GetIndex(), a_screenPos, a_screenRect, a_color, a_rotation, a_layer, a_uvOffset, a_pivot, a_uvScale,a_curveCenter,a_curveRadius,a_curveAngle);
	}

	void GraphicsEngine::SubmitUI(const Handle<Resource::Texture>& a_texHandle, const Math::Vector2& a_screenPos, float a_scale, const Math::Color& a_color, float a_rotation, float a_layer, const Math::Vector2& a_uvOffset, const Math::Vector2& a_pivot, const Math::Vector2& a_curveCenter,
		float a_curveRadius,
		float a_curveAngle)
	{
		auto& _resMgr = Resource::ResourceManager::Instance();

		// 読み込み中のものは描かない : 空の実体のサイズを掛けても意味がない
		if (!_resMgr.IsReady(a_texHandle)) return;

		auto* _pTex = _resMgr.Get(a_texHandle);
		if (!_pTex) return;

		// テクスチャの元サイズにスケールを掛けたものを表示サイズにする
		Math::Vector2 _size = { _pTex->GetDesc().Width * a_scale, _pTex->GetDesc().Height * a_scale };
		PushUIData(_pTex->GetSRV().GetIndex(), a_screenPos, _size, a_color, a_rotation, a_layer, a_uvOffset, a_pivot, {1.0f,1.0f}, a_curveCenter, a_curveRadius, a_curveAngle);
	}

	UINT GraphicsEngine::SetInstanceData(const MeshInstanceData& a_instanceData)
	{
		UINT _index = static_cast<UINT>(m_meshInstanceDataVec.size());
		m_meshInstanceDataVec.push_back(a_instanceData);
		return _index;
	}
	UINT GraphicsEngine::SetMeshMaterialData(const MeshMaterial& a_subsetData)
	{
		UINT _index = static_cast<UINT>(m_meshMaterialDataVec.size());
		m_meshMaterialDataVec.push_back(a_subsetData);
		return _index;
	}
	void GraphicsEngine::AddItem(const LightWeightDrawItem& a_item)
	{
		// アイテム配列に追加
		m_lightWeightDrawItemVec.push_back(a_item);
	}
	std::span<const LightWeightDrawItem> GraphicsEngine::GetPassItems(uint8_t a_passIndex)
	{
		// 探したいパスのキーの最小値と最大値を求める
		uint64_t _minKey = static_cast<uint64_t>(a_passIndex) << 56;
		uint64_t _maxKey = _minKey | 0x00FFFFFFFFFFFFFFull; // 下位56ビットをすべて1にする

		// ソート済み配列から開始位置を見つける
		auto _itStart = std::lower_bound(
			m_lightWeightDrawItemVec.begin(),
			m_lightWeightDrawItemVec.end(),
			_minKey,
			[](const LightWeightDrawItem& a_item, uint64_t a_value)
			{
				return a_item.sortKey.value < a_value;
			}
		);

		// ソート済み配列から終了位置を見つける
		auto _itEnd = std::upper_bound(
			_itStart,		// 開始位置から探す
			m_lightWeightDrawItemVec.end(),
			_maxKey,
			[](uint64_t a_value, const LightWeightDrawItem& a_item)
			{
				return a_value < a_item.sortKey.value;
			}
		);

		return std::span<const LightWeightDrawItem>(_itStart, _itEnd);
	}

	void GraphicsEngine::BindPSO(Graphics::RenderContext* a_pCtx, uint8_t a_psoIndex)
	{
		auto* _pPSO = m_pPipelineStateManager->GetPSO(a_psoIndex);
		if (!_pPSO) return;
		a_pCtx->SetGraphicPSO(_pPSO);
	}

	void GraphicsEngine::CreateGPUCameraData()
	{
		// リセット
		m_cbGPUCamera = {};

		// スクリーン座標を取得
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const auto& _renderingOp = Option::OptionManager::GetInstance().GetRenderingOption();

		// ジッターオフセット計算
		float _jitterX = 0.0f;
		float _jitterY = 0.0f;

		// ジッターオンオフ(グラフィックオプションで切り替え可能。OFFならジッター0でTAAはブレンドのみ)
		if (_renderingOp.useJitter)
		{
			// ハルトンシーケンスのテーブル（ピクセル中心地からのオフセット値 -0.5f ～ 0.5f）
			static const float _sHaltonX[16] = {
				0.000000f, -0.250000f,  0.250000f, -0.375000f,
				0.125000f, -0.125000f,  0.375000f, -0.437500f,
				0.062500f, -0.187500f,  0.312500f, -0.312500f,
				0.187500f, -0.062500f,  0.437500f, -0.468750f
			};
			static const float _sHaltonY[16] = {
				0.000000f,  0.166667f, -0.166667f,  0.500000f,
			   -0.500000f, -0.277778f,  0.055556f,  0.388889f,
			   -0.388889f, -0.055556f,  0.277778f,  0.444444f,
			   -0.222222f,  0.111111f, -0.444444f,  0.222222f
			};
			uint32_t _sampleIndex = m_totlaFrameCount % 16;

			// プロジェクション空間（NDC）のサイズに変換 : NDCは幅が２(-1～1)だから2倍
			_jitterX = (_sHaltonX[_sampleIndex] / (float)_winOp.windowWidth) * 2.0f;
			_jitterY = (_sHaltonY[_sampleIndex] / (float)_winOp.windowHeight) * 2.0f;
		}

		// カメラの行列を一時的に取得
		DXSM::Matrix _viewMat = m_cbCamera.viewMat;
		DXSM::Matrix _projMat = m_cbCamera.projMat;
		DXSM::Matrix _invViewMat = m_cbCamera.viewInvMat;
		DXSM::Matrix _invProjMat = m_cbCamera.projInvMat;

		// モーションベクター用のジッターなしViewProjを計算
		DXSM::Matrix _nonJitteredViewProj = _viewMat * _projMat;
		DXSM::Matrix _nonJitteredInvViewProj = _nonJitteredViewProj.Invert();

		// 描画用のジッターあり投影行列を作成
		DXSM::Matrix _jitteredProjMat = _projMat;
		_jitteredProjMat._31 += _jitterX;
		_jitteredProjMat._32 += _jitterY;

		// 描画用のジッターありViewProjとその逆行列を計算
		DXSM::Matrix _jitteredViewProj = _viewMat * _jitteredProjMat;
		DXSM::Matrix _invJitteredProj = _jitteredProjMat.Invert();
		DXSM::Matrix _invJitteredViewProj = _jitteredViewProj.Invert();

		// GPU転送用バッファへの詰め込み
		m_cbGPUCamera.pos = m_cbCamera.pos;

		// 通常の描画（SV_Positionの計算）にはジッターありを使う
		m_cbGPUCamera.viewMat = _viewMat.Transpose();
		m_cbGPUCamera.projMat = _jitteredProjMat.Transpose();
		m_cbGPUCamera.viewInvMat = _invViewMat.Transpose();
		m_cbGPUCamera.projInvMat = _invJitteredProj.Transpose();
		m_cbGPUCamera.viewProjMat = _jitteredViewProj.Transpose();
		m_cbGPUCamera.invViewProjMat = _invJitteredViewProj.Transpose();

		// モーションベクターの計算にはジッターなしを使う
		m_cbGPUCamera.nonJitteredProj = _projMat.Transpose();
		m_cbGPUCamera.nonJitteredViewProj = _nonJitteredViewProj.Transpose();
		m_cbGPUCamera.nonJitteredInvViewProj = _nonJitteredInvViewProj.Transpose();

		// 過去フレームのジッターなし行列の処理
		m_cbGPUCamera.prevView = m_prevViewMat.Transpose();
		m_cbGPUCamera.prevProj = m_prevProjMat.Transpose();
		m_cbGPUCamera.prevViewProj = m_prevNonJitteredViewProj.Transpose();

		// 次のフレームのためにジッターなしデータを保存
		m_prevViewMat = _viewMat;
		m_prevProjMat = _projMat;
		m_prevNonJitteredViewProj = _nonJitteredViewProj;

		// 完成したデータから、フラスタム平面を求める
		m_cbGPUCamera.ExtractFrustumPlanes(_nonJitteredViewProj);

		// フレームカウントを進める
		m_totlaFrameCount++;
	}
	void GraphicsEngine::ProcessInitQueue(D3D12::Device* a_pDevice, D3D12::GraphicsCommandList* a_pCmdList)
	{
		// ワールドと必須リソースの存在チェック
		auto* _pCurrentWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pCurrentWorld) return;

		if (!_pCurrentWorld->HasResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>()) return;
		if (!_pCurrentWorld->HasResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>()) return;

		auto& _initRequestVec = _pCurrentWorld->GetResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();
		if (_initRequestVec.empty()) return;

		auto& _dynamicPool = _pCurrentWorld->GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();

		// モデルのリソースからBLASと頂点バッファをコピー
		for (auto& _initReq : _initRequestVec)
		{
			// ターゲットとなるインスタンスデータと、ソースとなるモデルデータの取得
			auto* _pData = _dynamicPool.Ref(_initReq.dynamicInstanceHandle);
			auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_initReq.modelHandle);
			if (!_pData || !_pModel) continue;

			// モデル内の各メッシュごとに動的BLASを構築
			for (auto& _meshHandle : _pModel->GetMeshHandles())
			{
				// メッシュの有効性チェック
				auto* _pMesh = Resource::ResourceManager::Instance().Get(_meshHandle);
				if (!_pMesh || !_pMesh->HasRtData()) continue;

				// メッシュデータの追加と参照の取得
				_pData->meshDataVec.emplace_back();
				auto& _targetMeshData = _pData->meshDataVec.back();

				// インスタンス専用のアニメーション用頂点バッファ領域をメガバッファから割り当て
				UINT _vertexCount = _pMesh->GetRtData().vertexHandle.count;
				//_targetMeshData.animatedVertexHandle = m_animatedVertexBuffer.Allocate(_vertexCount);
				_targetMeshData.animatedVertexHandle = m_upMeshBufferAllocator->AllocateAnimatedVertex(_vertexCount);

				// サブメッシュ（マテリアル単位）ごとのジオメトリ情報を構築
				std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> _descVec = {};
				_descVec.reserve(_pMesh->GetMetaData().subsets.size());

				// レイトレーシング用データ作成
				for (auto& _subset : _pMesh->GetMetaData().subsets)
				{
					// ジオメトリ記述作成
					D3D12_RAYTRACING_GEOMETRY_DESC _desc = {};
					_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
					// 頂点バッファ
					_desc.Triangles.VertexBuffer.StartAddress =
						//m_animatedVertexBuffer.GetGPUVirtualAddress() +
						m_upMeshBufferAllocator->GetAnimatedVertexBuffer().GetGPUVirtualAddress() +
						(_targetMeshData.animatedVertexHandle.startIndex * sizeof(Resource::MeshVertexFloat));
					_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Resource::MeshVertexFloat);
					_desc.Triangles.VertexCount = _vertexCount;
					_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

					// インデックスバッファ
					_desc.Triangles.IndexBuffer =
						//m_meshIndexBuffer.GetGPUVirtualAddress() +
						m_upMeshBufferAllocator->GetIndexBuffer().GetGPUVirtualAddress() +
						sizeof(uint32_t) * (_subset.faceStart * 3 + _pMesh->GetRtData().indexHandle.startIndex);
					_desc.Triangles.IndexCount = _subset.faceCount * 3;
					_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

					_descVec.push_back(_desc);
				}
				_pData->meshDataVec.back().instanceBLAS.CreateDynamic(
					a_pDevice,
					a_pCmdList,
					_descVec
				);
				_pData->meshDataVec.back().meshHandle = _meshHandle;
			}



		}

		// 処理が終われば命令を解放
		_initRequestVec.clear();
	}
	int GraphicsEngine::GetSRVIndexFromTextureHandle(const Handle<Resource::Texture>& a_texHandle)
	{
		auto* _pTex = Resource::ResourceManager::Instance().Get(a_texHandle);
		ENGINE_ERRLOG(_pTex,"テクスチャが見つかりません");

		return static_cast<int>(_pTex->GetSRV().GetIndex());
	}

	bool GraphicsEngine::FetchDrawResources(
		const Resource::ModelDrawCommand& a_cmd,
		const Resource::Mesh*& a_pOutMesh,
		const Resource::Material*& a_pOutMaterial,
		const Resource::ShadingModelTable*& a_pOutShadingModel)
	{
		auto& _resManager = Resource::ResourceManager::Instance();

		// メッシュ
		a_pOutMesh = _resManager.Get(a_cmd.meshHandle);
		if (!a_pOutMesh) return false;

		// マテリアル
		a_pOutMaterial = _resManager.Get(a_cmd.materialHandle);
		if (!a_pOutMaterial) return false;

		// シェーディングモデル
		a_pOutShadingModel = _resManager.Get(a_pOutMaterial->shadingModelHandle);
		if (!a_pOutShadingModel) return false;

		return true;
	}

	MeshMaterial GraphicsEngine::BuildMeshMaterial(
		const Resource::Material* a_pMaterial,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd)
	{
		MeshMaterial _meshMaterial = {};
		_meshMaterial.baseColor = a_pMaterial->baseColor * a_albedoScale;
		_meshMaterial.emissive = a_pMaterial->emissive * a_emissiveScale;
		_meshMaterial.emissiveAdd = a_emissiveAdd;
		_meshMaterial.metallic = a_pMaterial->metallic;
		_meshMaterial.roughness = a_pMaterial->roughness;
		_meshMaterial.albedoIndex = GetSRVIndexFromTextureHandle(a_pMaterial->baseColorTex);
		_meshMaterial.metaRoughnessIndex = GetSRVIndexFromTextureHandle(a_pMaterial->metaRoughTex);
		_meshMaterial.emissiveIndex = GetSRVIndexFromTextureHandle(a_pMaterial->emissiveTex);
		_meshMaterial.normalIndex = GetSRVIndexFromTextureHandle(a_pMaterial->normalTex);
		return _meshMaterial;
	}

	void GraphicsEngine::RegisterDrawCommandToPasses(
		const Resource::ModelDrawCommand& a_cmd,
		const Resource::Mesh* a_pMesh,
		const Resource::Material* a_pMaterial,
		const Resource::ShadingModelTable* a_pShadingModel,
		const DXSM::Matrix& a_mat,
		const DXSM::Matrix& a_prevMat,
		bool a_isAnimation,
		uint32_t a_animatedVertexStart,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd,
		PSOKey a_psoKey)
	{
		for (UINT _passHash : a_pShadingModel->GetPassHashes())
		{
			auto* _pPassNode = m_upRenderGraph->GetPass(_passHash);
			if (!_pPassNode) continue;

			uint32_t _meshInstanceIdx = UINT32_MAX;

			// -----------------------------------------------------
			// メッシュシェーダー対応パスの場合のデータ構築
			// -----------------------------------------------------
			if (_pPassNode->pipelineBuilder.HasMeshShader())
			{
				a_psoKey.permutationFlags |= (uint32_t)Engine::Graphics::EShaderPermutationFlags::MeshShader;

				MeshMaterial _meshMaterial = BuildMeshMaterial(a_pMaterial, a_albedoScale, a_emissiveScale, a_emissiveAdd);

				const auto& _msData = a_pMesh->GetMeshShaderData();

				MeshInstanceData _meshInstanceData = {};
				_meshInstanceData.worldMat = a_mat.Transpose();
				_meshInstanceData.prevWorldMat = a_prevMat.Transpose();
				_meshInstanceData.materialOffset = SetMeshMaterialData(_meshMaterial);

				_meshInstanceData.meshletOffset = _msData.meshletHandle.startIndex + _msData.subsetMeshlets[a_cmd.subIdx].meshletOffset;
				_meshInstanceData.vertexOffset = a_pMesh->GetRtData().vertexHandle.startIndex;
				_meshInstanceData.uviOffset = _msData.uniqueVertexIndicesHandle.startIndex;
				_meshInstanceData.primitiveOffset = _msData.primitiveIndicesHandle.startIndex;

				_meshInstanceData.cullStart = _msData.cullDataHandle.startIndex + _msData.subsetMeshlets[a_cmd.subIdx].cullOffset;
				_meshInstanceData.meshletCount = a_pMesh->GetMeshShaderData().subsetMeshlets[a_cmd.subIdx].meshletCount;

				// アニメーション結果のスタートインデックス(非アニメ時は0)
				_meshInstanceData.animatedVertexStart = a_animatedVertexStart;
				_meshInstanceData.isAnimated = a_isAnimation ? 1 : 0;

				_meshInstanceIdx = SetInstanceData(_meshInstanceData);
			}

			// -----------------------------------------------------
			// 描画アイテム登録用のローカルヘルパー
			// -----------------------------------------------------
			auto AddDrawItemFunc = [&](const auto& a_psHandle)
				{
					a_psoKey.psHandle = a_psHandle;
					auto _psoHandle = _pPassNode->pipelineBuilder.Request(a_psoKey, m_upRenderGraph.get(), m_pPipelineStateManager);

					Engine::Graphics::LightWeightDrawItem _item = {};

					// 描画時に引き直すためのハンドル
					_item.meshHandle = a_cmd.meshHandle;
					_item.materialHandle = a_cmd.materialHandle;

					// ソートキーは同じ状態をまとめるためのものなので、添え字だけで足りる
					_item.sortKey.bits.meshID = a_cmd.meshHandle.GetIndex();
					_item.sortKey.bits.materialID = a_cmd.materialHandle.GetIndex();
					_item.isAnimation = a_isAnimation;
					_item.subIndex = a_cmd.subIdx;
					_item.meshInstanceIndex = _meshInstanceIdx;
					_item.subsetMeshletCount = a_pMesh->GetMeshShaderData().subsetMeshlets[a_cmd.subIdx].meshletCount;
					_item.sortKey.bits.psoID = static_cast<uint8_t>(_psoHandle.GetIndex());
					_item.sortKey.bits.passIndex = _pPassNode->passIndex;

					AddItem(_item);
				};

			// -----------------------------------------------------
			// シェーダーハンドルの展開と登録
			// -----------------------------------------------------
			auto _spanShaderHandles = a_pShadingModel->GetShaderHandles(_passHash);
			if (_spanShaderHandles.empty())
			{
				AddDrawItemFunc(Handle<Resource::Shader>()); // 空のハンドルで1回登録
			}
			else
			{
				for (const auto& _shader : _spanShaderHandles)
				{
					AddDrawItemFunc(_shader); // 各シェーダーごとに登録
				}
			}
		}
	}

	void GraphicsEngine::PushUIData(
		uint32_t a_texIndex,
		const Math::Vector2& a_pixelPos,
		const Math::Vector2& a_pixelSize,
		const Math::Color& a_color,
		float a_rotationDeg,
		float a_layer,
		const Math::Vector2& a_uvOffset,
		const Math::Vector2& a_pivot,
		const Math::Vector2& a_uvScale,
		const Math::Vector2& a_curveCenter,
		float a_curveRadius,
		float a_curveAngle
	)
	{
		// スクリーン解像度(px)
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return;

		// 回転(度→ラジアン)。回転はピクセル空間(等方)で行い、そのあとNDCへ変換する。
		// こうしないと、NDC空間(x,yで縮尺が違う)で回転させたときに斜めで画像が歪む。
		const float _rad = DirectX::XMConvertToRadians(a_rotationDeg);
		const float _cos = std::cos(_rad);
		const float _sin = std::sin(_rad);

		// 半サイズ(px)と、ピボット(正規化[0,1])からクアッド中心までのオフセット(px)。
		// (0.5 - pivot) * size がクアッド中心のピボットからのずれ。
		const DXSM::Vector2 _halfPx = { a_pixelSize.x * 0.5f, a_pixelSize.y * 0.5f };
		const DXSM::Vector2 _pivotOffPx = {
			(0.5f - a_pivot.x) * a_pixelSize.x,
			(0.5f - a_pivot.y) * a_pixelSize.y
		};

		// クアッド頂点 q∈[-1,1] に対し、ピクセル空間での最終座標は
		//   finalPx = centerPx + q.x*axisXpx + q.y*axisYpx
		// 回転はピボットを中心に行うので、centerPx = ピボット位置 + R*ピボットオフセット。
		//
		// ベースクアッドのUVは q.x=+1 がテクスチャ右、q.y=+1 がテクスチャ上(v=0)。
		// よって未回転時、ローカル+Xは画面右(+pixelX)、ローカル+Yは画面上(-pixelY)を向く。
		// この基底(+X=右, +Y=上)を回転行列 R(θ) で回す。
		//   axisXpx = R*( halfX,      0) = ( halfX*cos, halfX*sin)
		//   axisYpx = R*(     0, -halfY) = ( halfY*sin,-halfY*cos)
		const DXSM::Vector2 _centerPx = {
			a_pixelPos.x + (_pivotOffPx.x * _cos - _pivotOffPx.y * _sin),
			a_pixelPos.y + (_pivotOffPx.x * _sin + _pivotOffPx.y * _cos)
		};
		const DXSM::Vector2 _axisXpx = { _halfPx.x * _cos,  _halfPx.x * _sin };
		const DXSM::Vector2 _axisYpx = { _halfPx.y * _sin, -_halfPx.y * _cos };

		// ピクセル(左上原点/Y下向き) → NDC(中心原点/Y上向き)。
		// 点は原点シフトあり、方向ベクトルはスケールのみ(Yは符号反転)。
		UIData _data = {};
		_data.pos   = { _centerPx.x / _w * 2.0f - 1.0f, 1.0f - _centerPx.y / _h * 2.0f };
		_data.axisX = { _axisXpx.x * 2.0f / _w, -_axisXpx.y * 2.0f / _h };
		_data.axisY = { _axisYpx.x * 2.0f / _w, -_axisYpx.y * 2.0f / _h };
		_data.uvOffset = a_uvOffset;
		_data.uvScale = a_uvScale;
		_data.color = Math::DX::ToVector4(a_color);
		_data.layer = a_layer;
		_data.texIndex = a_texIndex;
		_data.curveCenter = a_curveCenter;
		_data.curveAngle = a_curveAngle;
		_data.curveRadius = a_curveRadius;
		m_uiDrawItemVec.push_back(_data);
	}
}