#include "SceneManager.h"

#include "../BaseScene/BaseScene.h"

#include "Engine/MainEngine.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../Editor/Editor.h"
#include "../../Editor/EffectEditor/EffectEditor.h"

namespace Engine::Scene
{
	void SceneManager::Release()
	{
		// エディターが覚えている選択はここで消えるシーンのもの
		Engine::Editor::MainEditor::Instance().OnSceneChanged();

		m_upBaseSceneVec.clear();
	}

	//======================================================================================
	// エフェクトエディターが開いているか
	//--------------------------------------------------------------------------------------
	// 開いている間はゲームのシーンを止め、あちらの確認用ワールドだけを回す。
	// エフェクト単体を見るための画面なので、後ろでゲームが動いていると
	// 描画も当たり判定も混ざってしまう。
	//======================================================================================
	namespace
	{
		Editor::EffectEditor* GetActiveEffectEditor()
		{
			auto* _pEffectEditor = Editor::MainEditor::Instance().RefEffectEditor();
			if (!_pEffectEditor || !_pEffectEditor->IsOpen()) return nullptr;
			return _pEffectEditor;
		}
	}

	void SceneManager::Update(float a_dt)
	{
		// エフェクト確認中はゲームのシーンを止める。
		// シーンの切り替え命令もここで消化しないので、閉じたあとに順番どおり流れる
		if (auto* _pEffectEditor = GetActiveEffectEditor())
		{
			_pEffectEditor->UpdateScene(a_dt);
			return;
		}

		// シーンの切り替え
		ChangeScenen();

		//==================================================================
		// シーンの更新
		//------------------------------------------------------------------
		// 既定は一番上のシーンだけ。重ねたシーン(ポーズ画面)を出している間、
		// 後ろのゲームは止まっていてほしいため。
		//
		// 描画(Draw)は積んであるシーンを全部通すので、止まっていても後ろは
		// 見えたままになる。
		//
		// 後ろも一緒に動かしたい重ね方をするときだけ、この切り替えを外す。
		//==================================================================
		if (m_isUpdateTopSceneOnly)
		{
			// 最前面のみ更新
			if (!m_upBaseSceneVec.empty())
			{
				m_upBaseSceneVec.back()->Update(a_dt);
			}
		}
		else
		{
			// すべてのシーンを更新
			for (auto& _scene : m_upBaseSceneVec)
			{
				_scene->Update(a_dt);
			}
		}
	}

	void SceneManager::Draw()
	{
		// エフェクト確認中は、あちらのワールドの描画命令だけをレンダーグラフへ流す。
		// レンダーグラフ自体はゲームと同じものを通るので、見え方は本番と揃う
		if (auto* _pEffectEditor = GetActiveEffectEditor())
		{
			_pEffectEditor->DrawScene();
			return;
		}

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

	//======================================================================================
	// 空のシーンを作る
	//======================================================================================
	Engine::GUID SceneManager::CreateEmptyScene(const std::string& a_path, const std::string& a_name)
	{
		if (a_name.empty())
		{
			ENGINE_WARNING("[Scene] 名前が空のためシーンを作成できません");
			return Engine::GUID();
		}

		//------------------------------------------------------------------
		// 置き場所を決める
		//
		// 「名前を付けて保存」と同じ並びにしておく。
		// シーンごとにフォルダを掘るのは、後から一緒に置きたいものが出てくるため
		//   Asset/Scenes/<サブフォルダ>/<名前>/<名前>.ojscene
		//------------------------------------------------------------------
		std::string _dirPath = "Asset/Scenes/";
		if (!a_path.empty()) _dirPath += a_path + "/";
		_dirPath += a_name;

		const std::string _basePath = _dirPath + "/" + a_name;

		// すでにないかチェック
		const Engine::GUID _checkGUID = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_WARNING("[Scene] すでに同じ名前のシーンがあります : %s", _basePath.c_str());
			return Engine::GUID();
		}

		std::error_code _errorCode = {};
		std::filesystem::create_directories(_dirPath, _errorCode);
		if (_errorCode)
		{
			ENGINE_WARNING("[Scene] フォルダを作成できません : %s", _dirPath.c_str());
			return Engine::GUID();
		}

		// アセットデータベースに場所を作る
		const Engine::GUID _guid = Resource::AssetDatabase::Instance().AddMetaData(_basePath, "Scene");

		//------------------------------------------------------------------
		// 空の中身を書き出す
		//
		// 開いているシーンには触らない。作るだけで、開くかどうかは呼び出し側が決める
		//------------------------------------------------------------------
		{
			BaseScene _emptyScene;
			_emptyScene.Enter();
			_emptyScene.SetGUID(_guid);

			Persistence::Archive _ar(Persistence::Archive::Mode::Save, _dirPath, a_name, "scene");
			_emptyScene.Archive(_ar);

			_emptyScene.Exit();
		}

		ENGINE_LOG("[Scene] 新規作成 : %s", _basePath.c_str());

		return _guid;
	}

	bool SceneManager::PushScene(const Engine::GUID& a_guid)
	{
		// シーンの新規作成 : GUIDからロードする
		auto _upScene = std::make_unique<BaseScene>();
		std::string _sceneFilePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_sceneFilePath.empty())
		{
			ENGINE_ERRLOG(false, "指定されたGUIDのシーンファイルが見つかりません");
			return false;
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

		return true;
	}
	//======================================================================================
	// 最前面のシーンを消す
	//--------------------------------------------------------------------------------------
	// 使われなくなったリソースの破棄も当たり判定の空間も全シーンで共有しているので、
	// 片付けてよいのはシーンが1つも残らなくなったときだけ。
	//
	// ポーズ画面のように重ねたシーンを外しただけで片付けてしまうと、
	//   ・後ろのシーンがまだ持っているつもりのものが消える
	//   ・後ろのシーンの静的コライダーが消える(登録は Start の一度きりなので戻らない)
	// といった形で、戻った先が壊れる。
	//
	// リソースの破棄をここで行うのは、参照が外れた瞬間に捨てると
	// 同じシーンの中で出し直すたびに読み直しが走ってしまうため。
	// シーンの中では読み込んだものを持ったままにして、切れ目でまとめて片付ける。
	//======================================================================================
	void SceneManager::PopScene()
	{
		if (m_upBaseSceneVec.empty()) return;

		// GPU待ち
		D3D12::D3D12Wrapper::Instance().WaitForFrame();

		// これを外すと1つも残らないか
		const bool _isLastScene = (m_upBaseSceneVec.size() == 1);

		m_upBaseSceneVec.back()->Exit();
		m_upBaseSceneVec.pop_back();

		// 後ろに何も残っていなければ、共有しているものをまとめて片付ける
		// (当たり判定の空間はワールドの持ち物なので、上の Exit で一緒に消えている)
		if (_isLastScene)
		{
			// 誰も持っていないリソースはここで破棄する
			Resource::ResourceManager::Instance().SweepUnusedAll();
		}
	}
	
	//======================================================================================
	// シーンの切り替え
	//--------------------------------------------------------------------------------------
	// 消してから読み込むので、読み込みに失敗すると「シーンが1つも無い」状態が残る。
	// そうなると現在のシーンを引く先が全部空振りし、以降のフレームで落ちる。
	//
	// 消す前に行き先が引けるかどうかを確かめて、引けなければ今のシーンを残す。
	// (行き先の指定漏れ・GUIDの消滅は設定ミスなので、気付けるように知らせる)
	//======================================================================================
	void SceneManager::ReplaceScene(const Engine::GUID& a_guid)
	{
		if (m_upBaseSceneVec.empty()) return;

		if (Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid).empty())
		{
			ENGINE_WARNING("[Scene] 切り替え先のシーンが見つかりません : %s 今のシーンを続けます",
				a_guid.String().c_str());
			return;
		}

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
		// シーンが1つも無い間(起動直後・全消去後)もここは呼ばれる。
		// 空のまま back() を取るとその場で落ちるので、呼び出し側へ nullptr を返す
		if (m_upBaseSceneVec.empty()) return nullptr;

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
				// 1つずつ Pop に通す。最後の1つを外したところで
				// 共有の当たり判定空間が空になる
				while (!m_upBaseSceneVec.empty())
				{
					PopScene();
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