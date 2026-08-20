#include "SceneManager.h"

#include "../BaseScene/BaseScene.h"

#include "Engine/MainEngine.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../Editor/Editor.h"

namespace Engine::Scene
{
	void SceneManager::Release()
	{
		// エディターが覚えている選択はここで消えるシーンのもの
		Engine::Editor::MainEditor::Instance().OnSceneChanged();

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
			if (!m_upBaseSceneVec.empty())
			{
				m_upBaseSceneVec.back()->Update(a_dt);
			}
		}


	}

	void SceneManager::Draw()
	{
		// すべてのシーンを描画
		for (auto& _scene : m_upBaseSceneVec)
		{
			// 命令のスタック
			_scene->Draw();
		}
	}

	void SceneManager::SetWorldInitCallback(std::function<void(Engine::ECS::World* a_pWorld)> a_callback)
	{
		m_worldInitCallback = a_callback;
	}

	void SceneManager::InvokeWorldInitCallback(Engine::ECS::World * a_pWorld)
	{
		if (m_worldInitCallback)
		{
			m_worldInitCallback(a_pWorld);
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

		// どのシーンを読み込むかをログ出力する
		ENGINE_LOG("[Scene] ロード : %s", _sceneFilePath.c_str());

		// シーンの初期化
		_upScene->Enter();

		// シーンの再構築
		auto _fileDir = Engine::File::GetDirFromPath(_sceneFilePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(_sceneFilePath);
		// 形式はビルドモード任せ(Auto)。Development までは .ojscene 優先、Shipping は .obscene のみ
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "scene");
		_upScene->Archive(_ar);
		_upScene->SetGUID(a_guid);
		// スタックに積む
		m_upBaseSceneVec.push_back(std::move(_upScene));
	}
	void SceneManager::PopScene()
	{
		if (m_upBaseSceneVec.empty()) return;

		// GPU待ち
		D3D12::D3D12Wrapper::Instance().WaitForFrame();

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
		if (m_upBaseSceneVec.empty()) return nullptr;

		return m_upBaseSceneVec.back()->RefWorld();
	}

	BaseScene* SceneManager::GetCurrentTopScene()
	{
		return m_upBaseSceneVec.back().get();
	}

	GameObject::GameObjectManager* SceneManager::RefGameObjectManager()
	{
		if (m_upBaseSceneVec.empty()) return nullptr;

		return m_upBaseSceneVec.back()->RefGameObjectManager();
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

			//----------------------------------------------------------------------
			// 実行前にエディターの選択を捨てる
			//
			// 選択中のエンティティIDもゲームオブジェクトのポインタも、
			// 今のシーンのワールド・オブジェクトマネージャーが持っているもの。
			// Pop/Replace/Clear では実体ごと消え、Push でも参照先のシーンが
			// 変わるため、どの切り替え方でも持ち越してはいけない。
			// (パネル側の検証は描画時にしか回らないので、ここで先に断つ)
			//----------------------------------------------------------------------
			Engine::Editor::MainEditor::Instance().OnSceneChanged();

			switch (_cmd.changeType)
			{
			case SceneChangeType::Push:
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