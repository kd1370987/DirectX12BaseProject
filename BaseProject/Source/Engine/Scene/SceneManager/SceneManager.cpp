#include "SceneManager.h"

#include "../BaseScene/BaseScene.h"

#include "Engine/MainEngine.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"

namespace Engine::Scene
{
	bool SceneManager::Init()
	{
		PushScene(Engine::GUID("b467bb54-09cf-4b59-93e9-87c7ba196050"));
		return true;
	}

	void SceneManager::Release()
	{
		m_upBaseSceneVec.clear();
	}

	void SceneManager::Update(float a_dt)
	{
		// シーンの切り替え
		ChangeScenen();

		// シーンの更新
		if (!m_isOneUpdate)
		{
			// すべてのシーンを更新
			for (auto& _scene : m_upBaseSceneVec)
			{
				_scene->Update(a_dt);
			}
		}
		else
		{
			// 最前面のみ更新
			auto& _scene = m_upBaseSceneVec.back();
			_scene->Update(a_dt);
		}


	}

	void SceneManager::Draw(Engine::Graphics::RenderContext* a_pRCT)
	{
		// すべてのシーンを描画
		for (auto& _scene : m_upBaseSceneVec)
		{
			Engine::Editor::MainEditor::Instance().StartWatch("ECSDraw");
			// 命令のスタック
			_scene->Draw();
			Engine::Editor::MainEditor::Instance().EndWatch("ECSDraw");

			// 命令の実行
			Engine::Editor::MainEditor::Instance().StartWatch("RGDraw");
			Engine::MainEngine::Instance().ExcuteDrawCmd();
			Engine::Editor::MainEditor::Instance().EndWatch("RGDraw");
		}
	}

	void SceneManager::PushScene(const Engine::GUID& a_guid)
	{
		// シーンの新規作成 : GUIDからロードする
		auto _upScene = std::make_unique<BaseScene>();
		std::string _sceneFilePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_sceneFilePath.empty())
		{
			ENGINE_ERRLOG(false, "指定されたGUIDのシーンファイルが見つかりません");
			return;
		}

		// シーンの初期化
		_upScene->Enter();

		// シーンの再構築
		auto _fileDir = FileUtility::GetDirFromPath(_sceneFilePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(_sceneFilePath);
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "scene");
		_upScene->Archive(_ar);

		// スタックに積む
		m_upBaseSceneVec.push_back(std::move(_upScene));
	}
	void SceneManager::PopScene()
	{
		if (m_upBaseSceneVec.empty()) return;

		m_upBaseSceneVec.back()->Exit();
		m_upBaseSceneVec.pop_back();
	}
	
	void SceneManager::ReplaceScene(const Engine::GUID& a_guid)
	{
		if (m_upBaseSceneVec.empty()) return;

		PopScene();
		PushScene(a_guid);
	}

	Engine::ECS::World* SceneManager::RefWorld()
	{
		return m_upBaseSceneVec.back()->RefWorld();
	}

	BaseScene* SceneManager::GetCurrentTopScene()
	{
		return m_upBaseSceneVec.back().get();
	}

	void SceneManager::SetNextScene(const Engine::GUID& a_guid, const SceneChangeType& a_changeType)
	{
		m_sceneChangeCmd.push({ a_guid,a_changeType });
	}

	void SceneManager::ChangeScenen()
	{
		// 命令がある間
		while (!m_sceneChangeCmd.empty())
		{
			auto& _cmd = m_sceneChangeCmd.front();
			switch (_cmd.changeType)
			{
			case SceneChangeType::Puch:
				PushScene(_cmd.sceneGUID);
				break;
			case SceneChangeType::Pop:
				PopScene();
				break;
			case SceneChangeType::Replace:
				ReplaceScene(_cmd.sceneGUID);
				break;
			case SceneChangeType::Clear:
				while (!m_upBaseSceneVec.empty())
				{
					m_upBaseSceneVec.back()->Exit();
					m_upBaseSceneVec.pop_back();
				}
				break;
			default:
				break;
			}

			// 命令消去
			m_sceneChangeCmd.pop();
		}
	}


	// コンストラクタ・デストラクタ
	SceneManager::SceneManager()
	{}
	SceneManager::~SceneManager()
	{}
}