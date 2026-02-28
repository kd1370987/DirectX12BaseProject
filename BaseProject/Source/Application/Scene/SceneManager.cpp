#include "SceneManager.h"

#include "BaseScene/BaseScene.h"
#include "GameScene/GameScene.h"
#include "TitleScene/TitleScene.h"

bool SceneManager::Init()
{
	// シーン登録
	RegisterScene<GameScene>(SceneType::Game);
	RegisterScene<TitleScene>(SceneType::Title);

	// 初回シーン
	PushScene(SceneType::Game);

	return true;
}

void SceneManager::Release()
{
	m_sceneCreateFuncMap.clear();
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

void SceneManager::Draw()
{
	// すべてのシーンを描画
	for (auto& _scene : m_upBaseSceneVec)
	{
		_scene->Draw();
	}
}

void SceneManager::PushScene(const SceneType& a_sceneType)
{
	auto _it = m_sceneCreateFuncMap.find(a_sceneType);
	if (_it == m_sceneCreateFuncMap.end())
	{
		assert(0 && "シーンは未登録です");
		return;
	}

	auto _upScene = _it->second();
	_upScene->Enter();

	m_upBaseSceneVec.push_back(std::move(_upScene));
}
void SceneManager::PopScene()
{
	if (m_upBaseSceneVec.empty()) return;

	m_upBaseSceneVec.back()->Exit();
	m_upBaseSceneVec.pop_back();
}
void SceneManager::ReplaceScene(const SceneType& a_sceneType)
{
	if (m_upBaseSceneVec.empty()) return;

	PopScene();
	PushScene(a_sceneType);
}

World* SceneManager::RefWorld()
{
	return m_upBaseSceneVec.back()->RefWorld();
}

void SceneManager::SetNextScene(const SceneType& a_nextScene, const SceneChangeType& a_changeType)
{
	m_sceneChangeCmd.push({a_nextScene,a_changeType});
}

void SceneManager::ChangeScenen()
{
	// 命令がある間
	while (!m_sceneChangeCmd.empty())
	{
		auto& _cmd = m_sceneChangeCmd.front();
		switch (_cmd.changeType)
		{
		case SceneChangeType::Puch : 
			PushScene(_cmd.type);
			break;
		case SceneChangeType::Pop:
			PopScene();
			break;
		case SceneChangeType::Replace:
			ReplaceScene(_cmd.type);
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
{
}
SceneManager::~SceneManager()
{
}
