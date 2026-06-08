#include "MainEngine.h"

#include "Engine/Window/NativeWindow.h"
#include "Engine/TimeManager/TimeManager.h"
#include "Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Resource/Manager/ResourceManager/ResourceManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

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
		_desc.width = m_config.GetRuntimeConfig().windowWidth;
		_desc.height = m_config.GetRuntimeConfig().windowHeight;
		_desc.titleName = L"DirectX12";
		_desc.className = L"AppWindow";
		_desc.windowMode = Window::EWindowMode::Windowed;
		if (!m_upWindow->Create(_desc))
		{
			assert(0 && "ウィンドウ作成失敗");
			return;
		}

		// タイムマネージャークラスの生成
		m_upTimeManager = std::make_unique<Time::TimeManager>();
		m_upTimeManager->Init(m_config.GetRuntimeConfig().targetFrameRate);

		// アセットマネージャー作成
		InitializeAssetDatabase();


		// DirectX12関連オブジェクトの初期化
		if (!D3D12::D3D12Wrapper::Instance().Init(m_upWindow->GetWindowHandle(), m_upWindow->GetClientWidth(), m_upWindow->GetClientHeight()))
		{
			assert(0 && "D3D12Wrapperの初期化失敗");
			return;
		}

		// ディスクリプタヒープテーブルマネージャーの初期化
		if (!D3D12::DescriptorHeapManager::Instance().Init(100, 4000,100,100,10))
		{
			assert(0 && "ディスクリプタヒープマネージャーの初期化に失敗");
			return;
		}

		// リソースマネージャー
		Resource::ResourceManager::Instance().Init(
			D3D12::D3D12Wrapper::Instance().GetDevice(),
			D3D12::D3D12Wrapper::Instance().GetCopyCommandQueue()
		);

		// パイプラインステート・ルートシグネチャ管理
		m_upPipelineStateManager = std::make_unique<D3D12::PipelineStateManager>();
		m_upPipelineStateManager->Init(D3D12::D3D12Wrapper::Instance().GetDevice());

		// バックバッファの生成
		if (!D3D12::D3D12Wrapper::Instance().CreateRenderTarget())
		{
			assert(0 && "バックバッファの生成に失敗");
			return;
		}

		// 描画周り初期化
		m_upGraphicsEngine = std::make_unique<Graphics::GraphicsEngine>();
		Graphics::GraphicsEngineDesc _geDesc = {};
		_geDesc.width = m_config.GetRuntimeConfig().windowWidth;
		_geDesc.height = m_config.GetRuntimeConfig().windowHeight;
		_geDesc.pPipelineStateManager = m_upPipelineStateManager.get();
		m_upGraphicsEngine->Init(_geDesc);

		// レイトレワールド構築
		Engine::Raytracing::RayEngine::Instance().CommitWorld();

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
	}

	void MainEngine::Release()
	{

		// 設定を保存
		Option::OptionManager::GetInstance().Serialize();

		// 1. 【最重要】GPU同期待ち（すべての処理を確実に終わらせる）
		for (UINT _i = 0; _i < static_cast<UINT>(CPU_FRAME_COUNT); ++_i)
		{
			D3D12::D3D12Wrapper::Instance().WaitRender(_i);
		}

		// 2. アプリケーション・上位層の解放
		m_upCollisionWorld.reset(); // コリジョン解放

		// 3. リソースの解放 (ResourceManager内でテクスチャやバッファをResetしていると仮定)
		//Resource::ResourceManager::Instance().Release(); // ★追加（名前は実装に合わせてください）

		// 4. エディター（ImGui）解放
		Engine::Editor::MainEditor::Instance().Release();

		// 5. グラフィックスエンジンの解放（RenderContextなどが持つリソースを解放）
		m_upGraphicsEngine.reset(); // ★追加

		// 6. パイプラインステート・ルートシグネチャの解放 (ここで警告の572, 577が消えるはず)
		m_upPipelineStateManager.reset(); // ★追加

		// 7. ディスクリプタヒープマネージャー解放
		D3D12::DescriptorHeapManager::Instance().Release();

		// 8. 描画エンジン (D3D12Wrapper) の解放（デバイスは最後に死ぬ）
		D3D12::D3D12Wrapper::Instance().Shutdown();

		// 9. その他の解放
		m_upTimeManager->Release();
		m_upWindow->Release();

		// 10. 解放時にエラー検出（一番最後に呼ぶ）
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

		// リソース
		Resource::ResourceManager::Instance().Update();

		return true;
	}

	void MainEngine::EndFrame()
	{
		// フレーム終了
		m_upTimeManager->EndFrame(m_config.GetRuntimeConfig().isVsync);
	}

	void MainEngine::BeginDraw()
	{
		// 描画開始
		D3D12::D3D12Wrapper::Instance().BeginFrame();

		// 描画フレームリソース
		m_upGraphicsEngine->BegineFrame();

		// 当たり判定構築
		m_upCollisionWorld->BuildWorld();

		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetCommandList();
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
			m_upGraphicsEngine->GetRenderContext()->GetCBV_SRV_UAVHeap(),
		};
		_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

		// レイワールドインスタンスのクリア
		Raytracing::RayEngine::Instance().EndFrame();
	}

	void MainEngine::EndDraw()
	{
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetCommandList();

		Editor::MainEditor::Instance().StartWatch("EditorPhase");

		// ゲームモード以外の処理
		if (m_config.GetRuntimeConfig().appMode != EAppMode::Game)
		{
			// ディスクリプタヒープをセット
			ID3D12DescriptorHeap* _heaps[] = {
					D3D12::DescriptorHeapManager::Instance().GetImGuiHeap()
			};
			_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);
			// エディター描画
			Engine::Editor::MainEditor::Instance().Draw(D3D12::D3D12Wrapper::Instance().GetCommandList(),m_upWindow->GetClientWidth(),m_upWindow->GetClientHeight());
		}

		m_upGraphicsEngine->EndFrame();

		Editor::MainEditor::Instance().EndWatch("EditorPhase");

		Editor::MainEditor::Instance().StartWatch("EndFramePhase");
		// 描画終了
		D3D12::D3D12Wrapper::Instance().EndFrame(m_config.GetRuntimeConfig().isVsync);

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
	void MainEngine::InitializeAssetDatabase()
	{

		Resource::AssetDatabase::Instance().Init(
			"Asset/",			// クロールフォルダ指定
			".assetmeta"		// 作成拡張子
		);
		
		// ---- 対応する拡張子を登録 ----
		// 独自形式はバイナリのほうを基準とする
		// モデル
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Model",".gltf");
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Model",".fbx");
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Model",".obj");
		Resource::AssetDatabase::Instance().AddTypeExtensions("Model",".obmdl");
		Resource::AssetDatabase::Instance().AddTypeExtensions("Model",".ojmdl");
		// メッシュ
		Resource::AssetDatabase::Instance().AddTypeExtensions("Mesh", ".obmesh");
		Resource::AssetDatabase::Instance().AddTypeExtensions("Mesh", ".ojmesh");
		// マテリアル
		Resource::AssetDatabase::Instance().AddTypeExtensions("Material", ".obmtrl");
		Resource::AssetDatabase::Instance().AddTypeExtensions("Material", ".ojmtrl");
		// アニメーション
		Resource::AssetDatabase::Instance().AddTypeExtensions("Animation", ".obanim");
		Resource::AssetDatabase::Instance().AddTypeExtensions("Animation", ".ojanim");
		// テクスチャ
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Texture",".png");
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Texture",".jpg");
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Texture",".tga");
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Texture",".dds");
		// シェーダー
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Shader",".hlsl");
		Resource::AssetDatabase::Instance().AddSupporedExtensions("Shader",".cso");

		// 全アセットに一括でメタファイル作成
		// すでにあれば無視
		Resource::AssetDatabase::Instance().CreateMetaFileForAllAssets();

		// ランタイムデータ作成
		Resource::AssetDatabase::Instance().CreateRuntimeData();
	}
}