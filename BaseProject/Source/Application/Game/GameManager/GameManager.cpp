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

// App
#include "../UserData/UserData.h"
#include "../InputActions/InputManager/InputActionManager.h"

namespace App::Game
{
	//======================================================================================
	// ゲーム設定ファイルの置き場所
	//
	// 持っているのは「起動時に立ち上げるシーン」だけ。アセットではないので
	// AssetDatabase には載せず、エンジン設定(EngineData)と同じく Asset/Data の下へ直接置く。
	//======================================================================================
	namespace
	{
		constexpr const char* GameSettingDir  = "Asset/Data/Game";
		constexpr const char* GameSettingName = "GameData";
		constexpr const char* GameSettingExt  = "gmdt";
	}

	void App::Game::GameManager::Init()
	{
		// ゲーム設定(起動時に立ち上げるシーン)の読み込み
		LoadGameSetting();

		// ユーザーデータの復元
		if (!m_upUserData)
		{
			m_upUserData = std::make_unique<UserData>();
			m_upUserData->Load();
		}

		// 入力設定の復元
		if (!m_upInputActionManager)
		{
			m_upInputActionManager = std::make_unique<Input::InputActionManager>();
			m_upInputActionManager->Init(m_upUserData.get());
		}

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
			_objRegistry.RegisterType<App::Object::UIButton>("UIButton");						// 押せるUI。押されて何をするかは SetOnClick で外から差し込む
			_objRegistry.RegisterType<App::Object::UIImage>("UIImage");							// 置くだけの画像(タイトルの背景など)
			_objRegistry.RegisterType<App::Object::TitleSequence>("TitleSequence");				// タイトル画面の進行役。ボタンへ「押されたらシーンを切り替える」を差し込む
			_objRegistry.RegisterType<App::Object::SceneAmbientObject>("SceneAmbientObject");	// シーンの環境設定(環境光・平行光・フォグ・空)。シーンに1つ置く。
			_objRegistry.RegisterType<App::Object::ScoreHUD>("ScoreHUD");						// スコアの表示。数える側(ScoreSystem)とは分かれていて、ここは出すだけ
			_objRegistry.RegisterType<App::Object::ResultSequence>("ResultSequence");			// リザルト画面の進行役。ホームのボタンへ「押されたらタイトルへ」を差し込む
			_objRegistry.RegisterType<App::Object::HomeSequence>("HomeSequence");				// ホーム画面の進行役。ステージセレクト(一覧・詳細・出撃)と倉庫のボタンを束ねる
			_objRegistry.RegisterType<App::Object::PauseSequence>("PauseSequence");				// ポーズ画面の進行役。重ねたシーンを閉じる側(重ねるのは SceneSequence)
			_objRegistry.RegisterType<App::Object::MissionSelect>("MissionSelect");				// ミッションセレクト。ホームから出し入れされ、選ぶと確認ボックスを出して出撃する
			_objRegistry.RegisterType<App::Object::UIGauge>("UIGauge");							// ゲージ(HP / オーバーヒート / ブーストなど)。値は SetValue で外から入れる
			_objRegistry.RegisterType<App::Object::WaveAnnounceHUD>("WaveAnnounceHUD");			// ウェーブが出た合図(何番目かの表示と音)
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


		// 最初のシーンを挿入
		if (m_farstScene.IsValid())
		{
			Engine::Scene::SceneManager::Instance().SetNextScene(m_farstScene, Engine::Scene::SceneChangeType::Push);
		}
		else
		{
			ENGINE_ERRLOG(false,"初めのシーンが設定されていません");
		}

		// エディター関数登録
		Engine::Editor::MainEditor::Instance().RegisterEditFunc(
			[&]()
			{
				if (ImGui::Begin("GameSetting"))
				{
					DrawGameSettingEdit();
				}
				ImGui::End();
			}
		);
	}
	void GameManager::Update(float a_dt)
	{	
		ENGINE_PROFILE_SCOPE("GameUpdate");

		// シーンマネージャーの更新
		Engine::Scene::SceneManager::Instance().Update(a_dt);
	}
	void GameManager::Draw()
	{
		ENGINE_PROFILE_SCOPE("GameDraw");

		// シーンの描画 : 描画命令を積むだけで実行はしない
		Engine::Scene::SceneManager::Instance().Draw();
	}
	void GameManager::Release()
	{}
	void GameManager::FireGlobalEvent(const std::string & a_eventName)
	{}
	void GameManager::EditDraw()
	{

	}
	//======================================================================================
	// ゲーム設定の読み込み / 保存
	//
	// 中身は初回シーンのGUIDが1つだけ。エディターで選び直したら Save ボタンで書き戻す。
	//======================================================================================
	void GameManager::LoadGameSetting()
	{
		Engine::Persistence::Archive _arch(
			Engine::Persistence::Archive::Mode::Load,
			GameSettingDir, GameSettingName, GameSettingExt);

		_arch.Field("m_farstScene", m_farstScene);
	}
	void GameManager::SaveGameSetting()
	{
		Engine::Persistence::Archive _arch(
			Engine::Persistence::Archive::Mode::Save,
			GameSettingDir, GameSettingName, GameSettingExt);

		_arch.Field("m_farstScene", m_farstScene);
	}
	//======================================================================================
	// ゲーム設定の編集UI
	//
	// 選んだ時点では覚えるだけで、ファイルへ残すのは Save ボタン
	//======================================================================================
	void GameManager::DrawGameSettingEdit()
	{
		ImGui::Text("Farst Scene : %s", m_farstScene.String().c_str());

		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"##FarstScene",
			"Scene",
			m_farstScene);

		ImGui::Separator();

		if (ImGui::Button("Save"))
		{
			SaveGameSetting();
		}
	}
	GameManager::GameManager()
	{}
	GameManager::~GameManager()
	{}
}