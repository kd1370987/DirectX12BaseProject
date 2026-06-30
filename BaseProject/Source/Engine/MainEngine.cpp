#include "MainEngine.h"

#include "Engine/Window/NativeWindow.h"
#include "Engine/Time/TimeManager.h"
#include "Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Resource/Manager/ResourceManager/ResourceManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "Engine/Particle/ParticleBufferManager.h"

#include "Application/App.h"

#include "Collision/CollisionWorld.h"

#include "Option/OptionManager.h"
namespace Engine
{
	MainEngine::MainEngine()
	{}

	MainEngine::~MainEngine()
	{}

	void MainEngine::Init(EngineConfig a_config)
	{
		// 設定を保存
		m_config = a_config;

		// 設定を取得
		Option::OptionManager::GetInstance().Deserialize();
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

		// DirectX12でGPUの詳細なエラーを確認するためのもの
		if (a_config.GetInitConfig().isDebugLayer)
		{
			ComPtr<ID3D12Debug> _debug;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debug))))
			{
				_debug->EnableDebugLayer();
			}
		}

		

		// ウィンドウクラスの生成
		m_upWindow = std::make_unique<Window::NativeWindow>();
		Window::WindowDesc _desc = {};
		_desc.width = static_cast<UINT>(_winOp.windowWidth);
		_desc.height = static_cast<UINT>(_winOp.windowHegiht);
		_desc.titleName = L"DirectX12";
		_desc.className = L"AppWindow";
		_desc.windowMode = _winOp.windowMode;
		if (!m_upWindow->Create(_desc))
		{
			assert(0 && "ウィンドウ作成失敗");
			return;
		}

		// タイムマネージャークラスの生成
		m_upTimeManager = std::make_unique<Time::TimeManager>();
		m_upTimeManager->Init(static_cast<int>(_winOp.targetFrameRate));

		// DirectX12関連オブジェクトの初期化
		D3D12::D3D12Wrapper::Instance().Init(m_upWindow->GetWindowHandle(), m_upWindow->GetClientWidth(), m_upWindow->GetClientHeight());
		auto* _pDev = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();

		// アセットマネージャー作成
		InitializeAssetDatabase();

		// ディスクリプタヒープテーブルマネージャーの初期化
		if (!D3D12::DescriptorHeapManager::Instance().Init(100, 4000,100,100,10))
		{
			assert(0 && "ディスクリプタヒープマネージャーの初期化に失敗");
			return;
		}

		// パイプラインステート・ルートシグネチャ管理
		m_upPipelineStateManager = std::make_unique<D3D12::PipelineStateManager>();
		m_upPipelineStateManager->Init(D3D12::D3D12Wrapper::Instance().GetDevice());

		// バックバッファの生成
		D3D12::D3D12Wrapper::Instance().CreateBackBuffer();

		// 描画周り初期化
		m_upGraphicsEngine = std::make_unique<Graphics::GraphicsEngine>();
		Graphics::GraphicsEngineDesc _geDesc = {};
		_geDesc.width = static_cast<UINT>(_winOp.windowWidth);
		_geDesc.height = static_cast<UINT>(_winOp.windowHegiht);
		_geDesc.pPipelineStateManager = m_upPipelineStateManager.get();
		m_upGraphicsEngine->Init(_pCmdList,_geDesc);

		// パーティクルブッファの生成
		m_upParticleManager = std::make_unique<Particle::ParticleBufferManager>();
		m_upParticleManager->Init(_pDev,_pCmdList);

		// レイトレワールド構築
		Engine::Raytracing::RayEngine::Instance().CommitWorld(_pDev,_pCmdList);

		// エディター初期化
		if (!Engine::Editor::MainEditor::Instance().Init(m_upWindow->GetWindowHandle()))
		{
			assert(0 && "エディターの初期化に失敗");
			return;
		}

		m_config = a_config;

		// コリジョンワールド構築
		m_upCollisionWorld = std::make_unique<Collision::CollisionWorld>();
		m_upCollisionWorld->Clear();

		// ダイレクトキューの実行
		D3D12::D3D12Wrapper::Instance().CloseAndExecuteComdLists(_pCmdList);

	}

	void MainEngine::Release()
	{
		// 設定を保存
		Option::OptionManager::GetInstance().Serialize();

		// アプリケーション・上位層の解放
		m_upCollisionWorld.reset(); // コリジョン解放

		// リソースの解放
		Resource::ResourceManager::Instance().Release();

		// エディター（ImGui）解放
		Engine::Editor::MainEditor::Instance().Release();

		// グラフィックスエンジンの解放（RenderContextなどが持つリソースを解放）
		m_upGraphicsEngine->Release();
		m_upGraphicsEngine.reset();

		// パイプラインステート・ルートシグネチャの解放
		m_upPipelineStateManager->Release();
		m_upPipelineStateManager.reset();

		// ディスクリプタヒープマネージャー解放
		D3D12::DescriptorHeapManager::Instance().Release();

		// 描画エンジンの解放
		D3D12::D3D12Wrapper::Instance().Release();

		// その他の解放
		m_upTimeManager->Release();
		m_upWindow->Release();

		// 解放時にエラー検出（一番最後に呼ぶ）
		if (m_config.GetInitConfig().isDebugLayer)
		{
			ComPtr<IDXGIDebug1> debug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
			{
				debug->ReportLiveObjects(
					DXGI_DEBUG_ALL,
					DXGI_DEBUG_RLO_DETAIL
				);
			}
		}
	}

	bool MainEngine::BegineFrame()
	{
		// フレーム開始
		m_upTimeManager->BeginFrame();

		// メッセージ処理
		if (!m_upWindow->ProcessMessage())
		{
			return false;
		}

		// タイトル文字列変更
		std::string _str = "DX12_FrameWork FPS = " + std::to_string(m_upTimeManager->GetNowFPS()) +
			"; DELTATIME = " + std::to_string(m_upTimeManager->GetDeltaTime()) + ";";
		m_upWindow->ChangeTitle(_str);

		// 入力更新
		Input::InputManager::Instance().Update();

		

		m_upParticleManager->BeginFrame();					// パーティクルデータの更新

		return true;
	}

	void MainEngine::EndFrame()
	{
		// フレーム終了
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		m_upTimeManager->EndFrame(_winOp.isVsync);
	}

	void MainEngine::BeginDraw()
	{
		// 今から使うフレームの登録されているファンクションを実行して空にする
		UINT _currentFrameIdx = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();
		for (auto& _func : m_releaseQueues[_currentFrameIdx])
		{
			_func();
		}
		m_releaseQueues[_currentFrameIdx].clear();

		// 描画開始 : ここでフレームインデックスが更新される
		D3D12::D3D12Wrapper::Instance().BeginFrame();

		// 描画フレームリソース
		m_upGraphicsEngine->BegineFrame();

		// 当たり判定構築
		m_upCollisionWorld->BuildWorld();
		m_upCollisionWorld->DrawDebug();

		// レイワールドインスタンスのクリア
		Raytracing::RayEngine::Instance().EndFrame();
	}

	void MainEngine::EndDraw()
	{
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

		Editor::MainEditor::Instance().StartWatch("EditorPhase");

		// ゲームモード以外の処理
		if (m_config.GetRuntimeConfig().appMode != EAppMode::Game)
		{
			auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();
			// ディスクリプタヒープをセット
			ID3D12DescriptorHeap* _heaps[] = {
					D3D12::DescriptorHeapManager::Instance().GetImGuiHeap()
			};
			_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

			// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
			auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
				D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffarTex().GetRTV()
			);

			// レンダーターゲットを設定
			_pCmdList->OMSetRenderTargets(
				1,
				&_cpuHandle,
				FALSE,
				nullptr
			);

			// 新しいリストにビューポートとシザー矩形もセットする
			// ビューポートとシザー矩形を設定
			_pCmdList->RSSetViewports(1, &D3D12::D3D12Wrapper::Instance().GetViewport());
			_pCmdList->RSSetScissorRects(1, &D3D12::D3D12Wrapper::Instance().GetScissorRect());

			// エディター描画
			Engine::Editor::MainEditor::Instance().Draw(
				_pCmdList,
				_winOp.windowWidth,
				_winOp.windowHegiht
			);
			D3D12::D3D12Wrapper::Instance().SubmitDirectCommandList(_pCmdList);
		}

		m_upGraphicsEngine->EndFrame();

		Editor::MainEditor::Instance().EndWatch("EditorPhase");

		Editor::MainEditor::Instance().StartWatch("EndFramePhase");

		// 描画終了
		D3D12::D3D12Wrapper::Instance().EndFrame(_winOp.isVsync);

		Editor::MainEditor::Instance().EndWatch("EndFramePhase");
	}

	UINT MainEngine::GetFPS()
	{
		return m_upTimeManager->GetNowFPS();
	}

	float MainEngine::GetDeltaTime()
	{
		return m_upTimeManager->GetDeltaTime();
	}

	void MainEngine::ChangeMode(EAppMode a_mode)
	{
		m_config.RefRuntimeConfig().appMode = a_mode;
	}
	void MainEngine::ExcuteDrawCmd()
	{
		m_upGraphicsEngine->Excute();
	}
	const Window::NativeWindow* MainEngine::GetNativeWindow() const
	{
		return m_upWindow.get();
	}
	Window::NativeWindow* MainEngine::RefNativeWindow()
	{
		return m_upWindow.get();
	}
	Graphics::GraphicsEngine* MainEngine::RefGraphicsEngine()
	{
		return m_upGraphicsEngine.get();
	}
	const Graphics::RenderContext* MainEngine::GetRenderContext() const
	{
		return m_upGraphicsEngine->GetRenderContext();
	}
	Graphics::RenderContext* MainEngine::RefRenderContext()
	{
		return m_upGraphicsEngine->RefRenderContext();
	}
	Collision::CollisionWorld* MainEngine::RefCollisionWorld()
	{
		return m_upCollisionWorld.get();
	}
	const Particle::ParticleBufferManager* MainEngine::GetParticleManager() const
	{
		return m_upParticleManager.get();
	}
	Particle::ParticleBufferManager* MainEngine::RefParticleManager()
	{
		return m_upParticleManager.get();
	}
	const D3D12::PipelineStateManager* MainEngine::GetPipelineManager() const
	{
		return m_upPipelineStateManager.get();
	}
	D3D12::PipelineStateManager* MainEngine::RefPipelineManager()
	{
		return m_upPipelineStateManager.get();
	}
	void MainEngine::RegisterDeferredResource(std::function<void()> a_releaseFunc)
	{
		m_releaseQueues[D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex()].push_back(std::move(a_releaseFunc));
	}
	void MainEngine::InitializeAssetDatabase()
	{

		Resource::AssetDatabase::Instance().Init(
			"Asset/",			// クロールフォルダ指定
			".assetmeta"		// 作成拡張子
		);
		
		// ---- 対応する拡張子を登録 ----		
		// モデル
		Resource::TypeExtension _modelExt = {};
		_modelExt.type = "Model";
		_modelExt.typeExt.push_back(".obmdl");
		_modelExt.typeExt.push_back(".ojmdl");
		_modelExt.AddExtensions(".gltf");
		_modelExt.AddExtensions(".fbx");
		_modelExt.AddExtensions(".obj");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_modelExt);
		// メッシュ
		Resource::TypeExtension _meshExt = {};
		_meshExt.type = "Mesh";
		_meshExt.typeExt.push_back(".obmesh");
		_meshExt.typeExt.push_back(".ojmesh");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_meshExt);
		// マテリアル
		Resource::TypeExtension _materialExt = {};
		_materialExt.type = "Material";
		_materialExt.typeExt.push_back(".obmtrl");
		_materialExt.typeExt.push_back(".ojmtrl");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_materialExt);
		// アニメーション
		Resource::TypeExtension _animationExt = {};
		_animationExt.type = "Animation";
		_animationExt.typeExt.push_back(".obanim");
		_animationExt.typeExt.push_back(".ojanim");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_animationExt);
		// ステートマシン
		Resource::TypeExtension _stateExt = {};
		_stateExt.type = "StateMachinAsset";
		_stateExt.typeExt.push_back(".obstet");
		_stateExt.typeExt.push_back(".ojstet");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_stateExt);
		// パーティクル
		Resource::TypeExtension _particExt = {};
		_particExt.type = "ParticlesAsset";
		_particExt.typeExt.push_back(".obptic");
		_particExt.typeExt.push_back(".ojptic");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_particExt);
		// テクスチャ
		Resource::TypeExtension _texExt = {};
		_texExt.type = "Texture";
		_texExt.AddExtensions(".png");
		_texExt.AddExtensions(".jpg");
		_texExt.AddExtensions(".tag");
		_texExt.AddExtensions(".dds");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_texExt);
		// シェーダー
		Resource::TypeExtension _shaderExt = {};
		_shaderExt.type = "Shader";
		_shaderExt.AddExtensions(".hlsl");
		_shaderExt.AddExtensions(".cso");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_shaderExt);
		Resource::TypeExtension _shaderLibraryExt = {};
		_shaderLibraryExt.type = "ShaderLibrary";
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_shaderLibraryExt);
		// シーン
		Resource::TypeExtension _sceneExt = {};
		_sceneExt.type = "Scene";
		_sceneExt.AddExtensions(".ojscene");
		_sceneExt.AddExtensions(".obscene");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_sceneExt);
		// シーン
		Resource::TypeExtension _shadingModelTable = {};
		_shadingModelTable.type = "ShadingModelTable";
		_shadingModelTable.AddExtensions(".ojsmtble");
		_shadingModelTable.AddExtensions(".obsmtble");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_shadingModelTable);
		// 全アセットに一括でメタファイル作成
		// すでにあれば無視
		Resource::AssetDatabase::Instance().CreateMetaFileForAllAssets();

		// ランタイムデータ作成
		Resource::AssetDatabase::Instance().CreateRuntimeData();
	}
}