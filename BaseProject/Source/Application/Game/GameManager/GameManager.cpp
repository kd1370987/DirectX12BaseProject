#include "GameManager.h"

// エンジン
#include "../../../Engine/MainEngine.h"

// シーン関係
#include "../../../Engine/Scene/SceneManager/SceneManager.h"

// ECS関係(ゲーム用のワールド)
#include "../../ECS/World/World.h"

// ECS外オブジェクト(クラスメタマネージャー / 登録するクラス)
#include "../../../Engine/GameObject/ObjectMetaRegistry/ObjectMetaRegistry.h"
#include "Application/Object/UI/CombatReticleHUD/CombatReticleHUD.h"
#include "Application/Object/UI/TargetBoxHUD/TargetBoxHUD.h"
#include "Application/Object/UI/AimReticleHUD/AimReticleHUD.h"
#include "Application/Object/UI/HitEffectHUD/HitEffectHUD.h"
#include "Application/Object/UI/ScoreHUD/ScoreHUD.h"
#include "../../Object/UI/WaveAnnounceHUD/WaveAnnounceHUD.h"
#include "Application/Object/Sequence/ResultSequence/ResultSequence.h"
#include "Application/Object/UI/MissileLockBoxHUD/MissileLockBoxHUD.h"
#include "Application/Object/UI/UIButton/UIButton.h"
#include "Application/Object/UI/UIImage/UIImage.h"
#include "Application/Object/UI/UIGauge/UIGauge.h"
#include "Application/Object/Sequence/TitleSequence/TitleSequence.h"
#include "Application/Object/Sequence/HomeSequence/HomeSequence.h"
#include "Application/Object/Sequence/PauseSequence/PauseSequence.h"
#include "Application/Object/Sequence/MissionSelect/MissionSelect.h"
#include "../../Object/Scene/SceneAmbientObject/SceneAmbientObject.h"
#include "Application/Object/Sequence/SceneSequence/SceneSequence.h"


// インプット
#include "Engine/Input/InputCollector/InputCollector.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindowsMouse/InputAxisForWindowsMouse.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForXInput/InputAxisForXInput.h"

#include "Engine/Input/InputDevice/Button/InputButtonForWindows/InputButtonForWindows.h"
#include "Engine/Input/InputDevice/Button/InputButtonForXInput/InputButtonForXInput.h"

// ゲームフロウ
#include "../GameFlowStateMachine/GameFlowStateMachine.h"

#include "../../../Engine/Audio/AudioManager.h"

namespace App::Game
{
	void App::Game::GameManager::Init()
	{

		// ゲームフロウの読み込み
		m_upGameFlowMachine = std::make_unique<GameFlowStateMachine>();
		m_upGameFlowMachine->Load("Asset/Scenes/Flow/Flow.scene");

		// テスト : 音源読み込み
		m_testHandle = Engine::Audio::AudioManager::Instance().RequestSoundInstance("Asset/Sound/TEST/test.wav");

		// ------------------------------------------------------------------
		// ECS外オブジェクト(GameObject)のクラスをメタマネージャーへ登録する。
		// 登録したクラスはシーンの保存/読み込みと、エディターの AddObject 一覧で使われる。
		// 新しいオブジェクトクラスを追加したら、ここに RegisterType を足すこと。
		//
		// タイプIDは第2引数の「登録名」のハッシュなので、どこへ足しても、
		// 並べ替えても既存シーンには影響しない。逆に登録名を変えるとIDが変わるため、
		// 名前を変えたいときは MigrateName で旧名を引き継がせること。
		// ------------------------------------------------------------------
		{
			auto& _objRegistry = Engine::GameObject::ObjectMetaRegistry::Instance();
			_objRegistry.RegisterType<App::Object::CombatReticleHUD>("CombatReticleHUD");
			_objRegistry.RegisterType<App::Object::TargetBoxHUD>("TargetBoxHUD");
			_objRegistry.RegisterType<App::Object::SceneSequence>("SceneSequence");
			_objRegistry.RegisterType<App::Object::AimReticleHUD>("AimReticleHUD");
			_objRegistry.RegisterType<App::Object::HitEffectHUD>("HitEffectHUD");
			_objRegistry.RegisterType<App::Object::MissileLockBoxHUD>("MissileLockBoxHUD");
			// 押せるUI。押されて何をするかは SetOnClick で外から差し込む
			_objRegistry.RegisterType<App::Object::UIButton>("UIButton");
			// 置くだけの画像(タイトルの背景など)
			_objRegistry.RegisterType<App::Object::UIImage>("UIImage");
			// タイトル画面の進行役。ボタンへ「押されたらシーンを切り替える」を差し込む
			_objRegistry.RegisterType<App::Object::TitleSequence>("TitleSequence");
			// シーンの環境設定(環境光・平行光・フォグ・空)。シーンに1つ置く。
			_objRegistry.RegisterType<App::Object::SceneAmbientObject>("SceneAmbientObject");
			// スコアの表示。数える側(ScoreSystem)とは分かれていて、ここは出すだけ
			_objRegistry.RegisterType<App::Object::ScoreHUD>("ScoreHUD");
			// リザルト画面の進行役。ホームのボタンへ「押されたらタイトルへ」を差し込む
			_objRegistry.RegisterType<App::Object::ResultSequence>("ResultSequence");
			// ホーム画面の進行役。ステージセレクト(一覧・詳細・出撃)と倉庫のボタンを束ねる
			_objRegistry.RegisterType<App::Object::HomeSequence>("HomeSequence");
			// ポーズ画面の進行役。重ねたシーンを閉じる側(重ねるのは SceneSequence)
			_objRegistry.RegisterType<App::Object::PauseSequence>("PauseSequence");
			// ミッションセレクト。ホームから出し入れされ、選ぶと確認ボックスを出して出撃する
			_objRegistry.RegisterType<App::Object::MissionSelect>("MissionSelect");
			// ゲージ(HP / オーバーヒート / ブーストなど)。値は SetValue で外から入れる
			_objRegistry.RegisterType<App::Object::UIGauge>("UIGauge");
			// ウェーブが出た合図(何番目かの表示と音)
			_objRegistry.RegisterType<App::Object::WaveAnnounceHUD>("WaveAnnounceHUD");
		}

		// ------------------------------------------------------------------
		// シーンが持つワールドの作り手を差し込む。
		//
		// エンジンは基盤の Engine::ECS::World としてしか触らないので、
		// 「どの種類のワールドを立てるか」はゲーム側のここが決める。
		// 何を登録するかは App::ECS::World::RegisterGameTypes が持っている
		// (中身は Application/ECS/World/WorldTypeRegister.cpp)。
		// ------------------------------------------------------------------
		Engine::Scene::SceneManager::Instance().SetWorldFactory(
			[]() -> std::unique_ptr<Engine::ECS::World>
			{
				return std::make_unique<App::ECS::World>();
			}
		);


		// キーボード
		{
			Engine::Input::InputCollector _keyboard;
			Engine::Input::InputButtonForWindows _add('T');
			_keyboard.AddButton("Add", std::make_shared<Engine::Input::InputButtonForWindows>(_add));
			Engine::Input::InputButtonForWindows _save('K');
			_keyboard.AddButton("Save", std::make_shared<Engine::Input::InputButtonForWindows>(_save));

			// 移動
			Engine::Input::InputAxisForWindows _move('W', 'D', 'S', 'A');
			_keyboard.AddAxis("Move", std::make_shared<Engine::Input::InputAxisForWindows>(_move));
			// ジャンプ
			Engine::Input::InputButtonForWindows _jump(VK_SPACE);
			_keyboard.AddButton("Jump", std::make_shared<Engine::Input::InputButtonForWindows>(_jump));
			// 急降下 : ジャンプ(上昇)の逆で、押している間は下向きの入力になる
			// (エディターの複数選択も LCtrl だが、あちらは ImGui 側で見ているので共存する)
			Engine::Input::InputButtonForWindows _dive(VK_LCONTROL);
			_keyboard.AddButton("Dive", std::make_shared<Engine::Input::InputButtonForWindows>(_dive));
			// ブースト
			Engine::Input::InputButtonForWindows _boost(VK_LSHIFT);
			_keyboard.AddButton("Boost", std::make_shared<Engine::Input::InputButtonForWindows>(_boost));
			// 視点
			Engine::Input::InputAxisForWindows _look(VK_UP, VK_RIGHT, VK_DOWN, VK_LEFT);
			_keyboard.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForWindows>(_look));


			Engine::Input::InputButtonForWindows _debugCamUp('E');
			_keyboard.AddButton("FreeCamUp", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamUp));
			Engine::Input::InputButtonForWindows _debugCamDown('Q');
			_keyboard.AddButton("FreeCamDown", std::make_shared<Engine::Input::InputButtonForWindows>(_debugCamDown));

			// テスト用ボタン
			Engine::Input::InputButtonForWindows _test('T');
			_keyboard.AddButton("Test", std::make_shared<Engine::Input::InputButtonForWindows>(_test));

			// シーン遷移用
			Engine::Input::InputButtonForWindows _scene('R');
			_keyboard.AddButton("Scene", std::make_shared<Engine::Input::InputButtonForWindows>(_scene));

			// ポーズ : ゲーム中はポーズ画面を重ね、ポーズ中は閉じて戻る。
			// 拾うのは重ねる側(SceneSequence)と閉じる側(PauseSequence)の2つで、
			// どちらも「一番上のシーン」しか更新されないので取り合いにならない
			Engine::Input::InputButtonForWindows _pause(VK_ESCAPE);
			_keyboard.AddButton("Pause", std::make_shared<Engine::Input::InputButtonForWindows>(_pause));

			// ---- マウスボタン ----
			// 武器 : 左クリックで左手、右クリックで右手。
			// 撃てるかどうかは武器側(GunStateComponent / GunShootSystem)の担当で、
			// ここで作るのは「押されている」という命令だけ
			Engine::Input::InputButtonForWindows _shootLeft(VK_LBUTTON);
			_keyboard.AddButton("ShootLeft", std::make_shared<Engine::Input::InputButtonForWindows>(_shootLeft));
			Engine::Input::InputButtonForWindows _shootRight(VK_RBUTTON);
			_keyboard.AddButton("ShootRight", std::make_shared<Engine::Input::InputButtonForWindows>(_shootRight));

			// UIのボタン押下。UIButton が既定で見に行くアクション名
			// (左手の武器と同じ左クリックだが、意味が別なので名前を分けておく)
			Engine::Input::InputButtonForWindows _uiClick(VK_LBUTTON);
			_keyboard.AddButton("UIClick", std::make_shared<Engine::Input::InputButtonForWindows>(_uiClick));

			// ミサイル : 押している間ターゲットを溜め、離すと一斉射
			// (デバッグカメラの FreeCamUp と同じ E キー。使う場面が別なので共存させる)
			Engine::Input::InputButtonForWindows _missile('E');
			_keyboard.AddButton("Missile", std::make_shared<Engine::Input::InputButtonForWindows>(_missile));

			Engine::Input::InputManager::Instance().AddDevice("Keyboard", std::make_unique<Engine::Input::InputCollector>(_keyboard));
		}
		// マウス
		{
			// 視点
			Engine::Input::InputCollector _mouse;
			_mouse.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForWindowsMouse>());

			Engine::Input::InputManager::Instance().AddDevice("Mouse", std::make_unique<Engine::Input::InputCollector>(_mouse));

		}
		// コントローラー
		{
			//Engine::Input::InputCollector _cont;
			//_cont.AddAxis("Look", std::make_shared<Engine::Input::InputAxisForXInput>(0,false));
			//_cont.AddAxis("Move", std::make_shared<Engine::Input::InputAxisForXInput>(0,true));

			//Engine::Input::InputManager::Instance().AddDevice("Controller", std::make_unique<Engine::Input::InputCollector>(_cont));
		}

		// 最初のシーンを挿入
		Engine::GUID _initScene = m_upGameFlowMachine->Start();
		if (_initScene != Engine::DefaultGUID)
		{
			Engine::Scene::SceneManager::Instance().SetNextScene(_initScene, Engine::Scene::SceneChangeType::Push);
		}
		else
		{
			ENGINE_ERRLOG(false,"初めのシーンが見つかりません");
		}

		// エディター関数登録
		Engine::Editor::MainEditor::Instance().RegisterEditFunc(
			[&]()
			{
				if (ImGui::Begin("GameFlowEdit"))
				{
					if(m_upGameFlowMachine)
					{
						m_upGameFlowMachine->EditImGui();
					}
				}
				ImGui::End();
			}
		);
	}
	void GameManager::Update(float a_dt)
	{	
		m_upGameFlowMachine->SetTrigger("ON_START");

		if (Engine::Input::InputManager::Instance().IsPress("Scene"))
		{
			m_upGameFlowMachine->SetTrigger("ToTitle");
		}

		if (Engine::Input::InputManager::Instance().IsPress("Test"))
		{
			auto* _pSoundInstance = Engine::Audio::AudioManager::Instance().RefInstance(m_testHandle);
			_pSoundInstance->Play();
		}
			
		// 遷移チェック
		Engine::GUID _nextScene;
		if (m_upGameFlowMachine->Evaluate(_nextScene))
		{
			// 遷移が発生したので、指定された新しいシーンをロード！
			Engine::Scene::SceneManager::Instance().SetNextScene(_nextScene, Engine::Scene::SceneChangeType::Replace);
		}

		// タイマー開始
		Engine::Editor::MainEditor::Instance().StartTimer("GameUpdate");
		
		// シーンマネージャーの更新
		Engine::Scene::SceneManager::Instance().Update(a_dt);

		// タイマーストップ
		Engine::Editor::MainEditor::Instance().StopTimer("GameUpdate");
	}
	void GameManager::Draw()
	{
		// タイマー開始
		Engine::Editor::MainEditor::Instance().StartTimer("GameDraw");

		// シーンの描画 : 描画命令を積むだけで実行はしない
		Engine::Scene::SceneManager::Instance().Draw();

		// タイマーストップ
		Engine::Editor::MainEditor::Instance().StopTimer("GameDraw");
	}
	void GameManager::Release()
	{}
	void GameManager::FireGlobalEvent(const std::string & a_eventName)
	{}
	void GameManager::EditDraw()
	{

	}
	GameManager::GameManager()
	{}
	GameManager::~GameManager()
	{}
}