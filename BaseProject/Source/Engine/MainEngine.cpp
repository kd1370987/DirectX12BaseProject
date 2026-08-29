#include "MainEngine.h"

#include "Engine/Window/NativeWindow.h"
#include "Graphics/MouseCursor/MouseCursor.h"
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


#include "Option/OptionManager.h"

#include "Editor/EditorCamera/EditorCamera.h"
#include "Editor/EffectEditor/EffectEditor.h"

#include "Audio/AudioManager.h"

#include "Input/InputManager/InputManager.h"

#include "JobSystem/JobSystem.h"

namespace Engine
{
	MainEngine::MainEngine()
	{}

	MainEngine::~MainEngine()
	{}

	void MainEngine::Init()
	{
		// オプションマネージャーの初期化と読込
		auto& _optionManager = Option::OptionManager::GetInstance();
		_optionManager.Init();
		_optionManager.Deserialize();
		const auto& _winOp = _optionManager.GetWindowOption();

		// 設定を保存
		m_appMode = EAppMode::Game;
		m_buildMode = _optionManager.GetBuildConfig().buildMode;

		// インプット初期化
		Input::InputManager::Instance().Init();
		bool _isD3DDebug = false;
		// ビルドモードによって、仕様を変更
		switch (m_buildMode)
		{
		case EBuildConfiguration::Debug:
		{
			ComPtr<ID3D12Debug> _debug;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debug))))
			{
				_debug->EnableDebugLayer();
			}
			// D3DデバッグON
			_isD3DDebug = true;
			ENGINE_LOG("[Option] : Debug モードでビルドされます");
			break;
		}
		case EBuildConfiguration::Development:
		{
			ENGINE_LOG("[Option] : Development モードでビルドされます");
			break;
		}
		case EBuildConfiguration::Shipping:
		{
			ENGINE_LOG("[Option] : Shipping モードでビルドされます");
			break;
		}
		default:
			break;
		}

		// ウィンドウクラスの生成
		m_upWindow = std::make_unique<Window::NativeWindow>();
		Window::WindowDesc _desc = {};
		_desc.width = static_cast<UINT>(_winOp.windowWidth);
		_desc.height = static_cast<UINT>(_winOp.windowHeight);
		_desc.titleName = Engine::String::ToWideString(_winOp.windowTitle);
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
		// バックバッファは「描画解像度」で作る。
		// レンダーグラフのテクスチャも windowWidth/Height 基準で、
		// 最終段(CopyToBackBufferPass)がバックバッファへ CopyResource するため
		// サイズを一致させておく必要がある。
		// クライアント領域へはスワップチェインの STRETCH で伸ばされる。
		D3D12::D3D12Wrapper::Instance().Init(
			m_upWindow->GetWindowHandle(),
			_isD3DDebug,
			static_cast<UINT>(_winOp.windowWidth),
			static_cast<UINT>(_winOp.windowHeight)
		);
		auto* _pDev = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();

		// ジョブシステム起動
		m_upJobSystem = std::make_unique<Thread::JobSystem>();
		{
			uint32_t _threadCount = std::thread::hardware_concurrency();
			_threadCount -= 5;
			m_upJobSystem->Init(_threadCount);
		}

		// 非同期ロードの実行先として登録する。
		// ResourceManager 側からエンジンのシングルトンを引かせないよう、ここで渡す
		Resource::ResourceManager::Instance().SetJobSystem(m_upJobSystem.get());

		// オーディオエンジンの初期化
		Audio::AudioManager::Instance().Init();

		// 保存されている音量を流し込む。
		// オプションの読み込みはこれより前に済んでいるが、
		// AudioManager がまだ無い状態では入れられないのでここで反映する
		Option::OptionManager::GetInstance().GetAudioOption().Apply();

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
		_geDesc.height = static_cast<UINT>(_winOp.windowHeight);
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

		// マウスカーソル
		m_upMouseCursor = std::make_unique<Graphics::MouseCursor>();
		m_upMouseCursor->Init();

		Engine::Editor::MainEditor::Instance().RegisterEditFunc(
			[this]()
			{
				if (m_upMouseCursor) m_upMouseCursor->DrawImGui();
			}
		);

		// ダイレクトキューの実行
		D3D12::D3D12Wrapper::Instance().CloseAndExecuteComdLists(_pCmdList);
	}

	void MainEngine::Release()
	{
		// 設定を保存
		Option::OptionManager::GetInstance().Serialize();

		// ジョブシステムの解放は最初に行う。
		// 走っているジョブはリソースやGPUリソースを触っているため、
		// それらを解放する前に必ずワーカーを止めきること
		Resource::ResourceManager::Instance().SetJobSystem(nullptr);
		m_upJobSystem->Release();

		// アプリケーション・上位層の解放

		// 自前カーソルが握っているテクスチャの参照を返す。
		// リソースの解放より前に手放しておくこと
		if (m_upMouseCursor)
		{
			m_upMouseCursor->Release();
			m_upMouseCursor.reset();
		}

		// 再生中のサウンドインスタンスを破棄。
		// SoundEffectInstance は生成元の SoundEffect(= Resource::Sound) を
		// 参照しているため、リソース解放より先に片付ける。
		Audio::AudioManager::Instance().ReleaseInstances();

		// リソースの解放（Sound = DirectX::SoundEffect もここで解放される）
		Resource::ResourceManager::Instance().Release();

		// オーディオエンジンの解放。
		// SoundEffect が AudioEngine を参照しているため、必ずリソース解放の後に行う。
		// シングルトンの破棄順は保証されないので、ここで明示的に解放しておくこと。
		Audio::AudioManager::Instance().Release();

		// エディター（ImGui）解放
		Engine::Editor::MainEditor::Instance().Release();

		// グラフィックスエンジンの解放（RenderContextなどが持つリソースを解放）
		m_upGraphicsEngine->Release();
		m_upGraphicsEngine.reset();

		// パーティクルのGPUバッファ解放。
		// これらはディスクリプタヒープにハンドルを持つため、
		// 必ず DescriptorHeapManager の解放より前に破棄する。
		if (m_upParticleManager)
		{
			m_upParticleManager->Release();
			m_upParticleManager.reset();
		}

		// レイトレワールド(TLAS/BLAS・各種バッファ)の解放。
		// シングルトンが握っていて自動破棄されないため明示的に解放する。
		Raytracing::RayEngine::Instance().Release();

		// パイプラインステート・ルートシグネチャの解放
		m_upPipelineStateManager->Release();
		m_upPipelineStateManager.reset();

		// ディスクリプタヒープマネージャー解放
		D3D12::DescriptorHeapManager::Instance().Release();

		// 遅延解放キューを空にする
		// 全GPU作業の完了を待ってから実行し、デバイスより先にリソースを解放しきる
		D3D12::D3D12Wrapper::Instance().WaitForFrame();
		for (auto& _releaseQueue : m_releaseQueues)
		{
			for (auto& _func : _releaseQueue)
			{
				_func();
			}
			_releaseQueue.clear();
		}

		// 描画エンジンの解放
		D3D12::D3D12Wrapper::Instance().Release();

		// その他の解放
		m_upTimeManager->Release();
		m_upWindow->Release();

		

		// ビルドモードによって、仕様を変更
		switch (m_buildMode)
		{
		case EBuildConfiguration::Debug:
		{
			// 解放時にエラー検出（一番最後に呼ぶ）
			ComPtr<IDXGIDebug1> debug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
			{
				debug->ReportLiveObjects(
					DXGI_DEBUG_ALL,
					DXGI_DEBUG_RLO_DETAIL
				);
			}
			break;
		}
		case EBuildConfiguration::Development:
		{
			break;
		}
		case EBuildConfiguration::Shipping:
		{
			break;
		}
		default:
			break;
		}
	}

	bool MainEngine::BeginFrame()
	{
		auto& _optionManager = Option::OptionManager::GetInstance();

		// フレーム開始
		m_upTimeManager->BeginFrame();

		// メッセージ処理
		if (!m_upWindow->ProcessMessage())
		{
			return false;
		}

		// タイトルにFPSを表示するかどうか
		if (_optionManager.GetWindowOption().isTitleFPS)
		{
			std::string _titleName = _optionManager.GetWindowOption().windowTitle;
			_titleName += std::string(": FPS = ") + std::to_string(m_upTimeManager->GetNowFPS());
			_titleName += std::string(": DELTATIME = ") + std::to_string(m_upTimeManager->GetDeltaTime());
			m_upWindow->ChangeTitle(_titleName);
		}

		// 入力更新
		Input::InputManager::Instance().Update();

		// オーディオ更新
		// 鳴り終わったワンショットの回収とデバイスロスト復帰を行うため、
		// 音を鳴らしていなくても毎フレーム呼ぶ必要がある
		Audio::AudioManager::Instance().Update();

		m_upParticleManager->BeginFrame();					// パーティクルデータの更新

		// 自前カーソルの位置決め。
		// 描くのは後(ゲームはUIパス / エディターはImGui)だが、どちらから描かれても
		// 同じ位置になるようここで一度だけ決める。
		// OSのカーソルを消してよいかもここで決まるのでウィンドウへ伝える
		if (m_upMouseCursor)
		{
			m_upMouseCursor->Update();
			if (m_upWindow)
			{
				m_upWindow->SetCursorHidden(m_upMouseCursor->IsHideOSCursor());
			}
		}


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
		// エディターの更新を入れる
		Editor::MainEditor::Instance().Update(GetDeltaTime());

		// 描画開始 : ここでフレームインデックスが更新され、そのフレームのGPU完了を待機する

		Engine::Editor::MainEditor::Instance().StartTimer("D3D12WrapperBeginFrame");
		D3D12::D3D12Wrapper::Instance().BeginFrame();
		Engine::Editor::MainEditor::Instance().StopTimer("D3D12WrapperBeginFrame");

		// GPU計測結果の読み戻し
		// タイムスタンプはGPUが実行し終えて初めて確定するので、
		// 上のGPU待機を抜けた直後がフレーム中で唯一の安全な読み出し地点になる
		Editor::MainEditor::Instance().CollectGPUProfileResult();

		// 今から使うフレームに登録されているファンクションを実行して空にする
		// BeginFrameの待機を終えた後に実行することで、このインデックスを前回使ったフレームの
		// GPU作業が完了していることが保証される
		UINT _currentFrameIdx = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();
		for (auto& _func : m_releaseQueues[_currentFrameIdx])
		{
			_func();
		}
		m_releaseQueues[_currentFrameIdx].clear();

		// 描画フレームリソース
		m_upGraphicsEngine->BeginFrame();

		// レイワールドインスタンスのクリア
		Raytracing::RayEngine::Instance().EndFrame();
	}

	void MainEngine::EndDraw()
	{
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

		Editor::MainEditor::Instance().StartTimer("EditorPhase");

		// ゲームモード以外の処理
		if (m_appMode != EAppMode::Game)
		{
			auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();
			// ディスクリプタヒープをセット
			ID3D12DescriptorHeap* _heaps[] = {
					D3D12::DescriptorHeapManager::Instance().GetImGuiHeap()
			};
			_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

			// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
			auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
				D3D12::D3D12Wrapper::Instance().GetCurrentBackBufferTex().GetRTV()
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
			Engine::Editor::MainEditor::Instance().Draw(_pCmdList);
			D3D12::D3D12Wrapper::Instance().SubmitDirectCommandList(_pCmdList);
		}

		m_upGraphicsEngine->EndFrame();

		Editor::MainEditor::Instance().StopTimer("EditorPhase");

		Editor::MainEditor::Instance().StartTimer("EndFramePhase");

		// 描画終了
		D3D12::D3D12Wrapper::Instance().EndFrame(_winOp.isVsync);

		Editor::MainEditor::Instance().StopTimer("EndFramePhase");
	}

	UINT MainEngine::GetFPS()
	{
		return m_upTimeManager->GetNowFPS();
	}

	float MainEngine::GetDeltaTime()
	{
		return m_upTimeManager->GetDeltaTime();
	}

	//======================================================================================
	// アプリのモード切り替え
	//--------------------------------------------------------------------------------------
	// 切り替えを跨いで入力を持ち越さない。押しっぱなしのキーやマウスの移動量が残ると、
	//   ゲーム側 : 切り替え前の移動入力のままプレイヤーが走り出す
	//   エディタ側: 右クリック押しっぱなし扱いでフリーカメラが回りっぱなしになる
	// といった形で出る。どちらも「切り替えた瞬間に一度捨てる」だけで断てる。
	//======================================================================================
	void MainEngine::ChangeMode(EAppMode a_mode)
	{
		// 同じモードを指定し続けても毎フレーム入力を捨てないようにする
		// (モード切替はキーを押している間ずっと呼ばれる作りのため)
		if (m_appMode == a_mode) return;

		m_appMode = a_mode;

		Editor::MainEditor::Instance().ResetInput();
		Input::InputManager::Instance().ResetInput();
	}
	void MainEngine::ExecuteDrawCmd()
	{
		// エディターモードならフリーカメラを割り込ませる
		// (実際の上書きは GraphicsEngine::Execute() 内、ECS側のカメラ設定が終わった後)
		bool _isOverride = false;

		// エフェクトエディターが開いているなら、そちらのカメラが最優先。
		// 描いているのがあちらの確認用ワールドなので、フリーカメラで見ても何も映らない
		{
			auto* _pEffectEditor = Editor::MainEditor::Instance().RefEffectEditor();
			DXSM::Matrix _camWorld = {};
			DXSM::Matrix _camProj = {};
			if (_pEffectEditor && _pEffectEditor->TryGetCameraOverride(_camWorld, _camProj))
			{
				m_upGraphicsEngine->SetCameraOverride(_camWorld, _camProj);
				_isOverride = true;
			}
		}

		if (!_isOverride && m_appMode == EAppMode::Editor)
		{
			auto* _pEditorCam = Editor::MainEditor::Instance().RefEditorCamera();
			if (_pEditorCam && _pEditorCam->IsEnable())
			{
				m_upGraphicsEngine->SetCameraOverride(
					_pEditorCam->GetWorldMatrix(),
					_pEditorCam->GetProjMatrix()
				);
				_isOverride = true;
			}
		}

		// ゲームモード、またはフリーカメラ無効ならECSのカメラをそのまま使う
		if (!_isOverride)
		{
			m_upGraphicsEngine->ClearCameraOverride();
		}

		m_upGraphicsEngine->Execute();
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
	Graphics::MouseCursor* MainEngine::RefMouseCursor()
	{
		return m_upMouseCursor.get();
	}
	Thread::JobSystem* MainEngine::RefJobSystem()
	{
		return m_upJobSystem.get();
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
		// アニメーター(アニメ用ステートマシン)
		Resource::TypeExtension _stateExt = {};
		_stateExt.type = "AnimatorAsset";
		_stateExt.typeExt.push_back(".obstet");
		_stateExt.typeExt.push_back(".ojstet");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_stateExt);
		// ゲームプレイ用ステートマシン
		Resource::TypeExtension _actionSmExt = {};
		_actionSmExt.type = "ActionStateMachineAsset";
		_actionSmExt.typeExt.push_back(".obasm");
		_actionSmExt.typeExt.push_back(".ojasm");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_actionSmExt);
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
		// プレハブ
		Resource::TypeExtension _prfb = {};
		_prfb.type = "Prefab";
		_prfb.AddExtensions(".ojprfb");
		_prfb.AddExtensions(".obprfb");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_prfb);
		// サウンド
		Resource::TypeExtension _sound = {};
		_sound.type = "Sound";
		_sound.AddExtensions(".wav");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_sound);
		// オーディオビヘイビア(始動/継続/終了の音をまとめたもの)
		Resource::TypeExtension _audioBehavior = {};
		_audioBehavior.type = "AudioBehavior";
		_audioBehavior.AddExtensions(".ojaudbhv");
		_audioBehavior.AddExtensions(".obaudbhv");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_audioBehavior);
		// エフェクト(パーティクル+メッシュをまとめたもの)
		Resource::TypeExtension _effect = {};
		_effect.type = "EffectAsset";
		_effect.AddExtensions(".ojeffect");
		_effect.AddExtensions(".obeffect");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_effect);
		// レンダリングパイプライン(レンダーグラフの設計図)
		Resource::TypeExtension _renderingPipeline = {};
		_renderingPipeline.type = "RenderingPipelineAsset";
		_renderingPipeline.AddExtensions(".ojrpipe");
		_renderingPipeline.AddExtensions(".obrpipe");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_renderingPipeline);
		// フォント : 変換を挟まず .ttf などをそのまま読む
		Resource::TypeExtension _font = {};
		_font.type = "Font";
		_font.AddExtensions(".ttf");
		_font.AddExtensions(".otf");
		_font.AddExtensions(".ttc");
		Resource::AssetDatabase::Instance().AddSupporedExtensions(_font);
		// 全アセットに一括でメタファイル作成
		// すでにあれば無視
		Resource::AssetDatabase::Instance().CreateMetaFileForAllAssets();

		// ランタイムデータ作成
		Resource::AssetDatabase::Instance().CreateRuntimeData();
	}
}