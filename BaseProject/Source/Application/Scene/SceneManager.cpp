#include "SceneManager.h"

#include "BaseScene/BaseScene.h"
#include "GameScene/GameScene.h"

bool SceneManager::Init()
{
	// シーン登録
	RegisterScene<GameScene>(SceneType::Game);

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


// コンストラクタ・デストラクタ
SceneManager::SceneManager()
{
}
SceneManager::~SceneManager()
{
}
