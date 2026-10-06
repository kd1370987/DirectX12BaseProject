#include "SceneManager.h"

#include "../BaseScene/BaseScene.h"
#include "../../ECS/World/World.h"	// unique_ptr<World> を扱うので完全型が要る

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../Audio/AudioManager.h"

#include "Engine/DevTool/IDevTool.h"

namespace Engine::Scene
{
	void SceneManager::Release()
	{
		// エディターが覚えている選択はここで消えるシーンのもの
		if (auto* _pDevTool = MainEngine::Instance().RefDevTool()) _pDevTool->OnSceneChanged();

		//----------------------------------------------------------------------------------
		// 上のシーンから順に、PopScene と同じく後始末を通して消す
		//
		// clear() だけだと Exit(= World::Release)が走らず、Release フェーズのシステムが
		// 返すはずのもの(動的BLAS・ポーズ領域・音など)がワールドごと直接壊れる。
		// 動的BLASは GPU が最後のフレームで読んでいる最中に最終解放され、
		// デバッグレイヤーが CORRUPTION で止まっていた(終了時にたまに落ちる原因)。
		//
		// 待つのは Present まで含めて : 終了時なのでキューを空にしてよい
		//----------------------------------------------------------------------------------
		if (!m_upBaseSceneVec.empty())
		{
			if (auto* _pGE = MainEngine::Instance().RefGraphicsEngine())
			{
				_pGE->RefRenderDevice()->WaitForGPUIdle();
			}
		}
		while (!m_upBaseSceneVec.empty())
		{
			m_upBaseSceneVec.back()->Exit();
			m_upBaseSceneVec.pop_back();
		}

		// 消えたシーンのテクスチャを指したままにせず、平行光の席も返す
		ApplySceneAmbient();
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
		// 開発ツールが確認用シーン(エフェクトの確認など)を回しているなら、その窓口を返す
		DevTool::IDevTool* RefActiveScenePreview()
		{
			auto* _pDevTool = MainEngine::Instance().RefDevTool();
			if (!_pDevTool || !_pDevTool->IsScenePreviewActive()) return nullptr;
			return _pDevTool;
		}
	}

	void SceneManager::Update(Resource::ResourceManager& a_resourceManager, float a_dt)
	{
		// エフェクト確認中はゲームのシーンを止める。
		// シーンの切り替え命令もここで消化しないので、閉じたあとに順番どおり流れる
		if (auto* _pDevTool = RefActiveScenePreview())
		{
			_pDevTool->UpdateScenePreview(a_dt);
			return;
		}

		// シーンの切り替え
		ApplyReservedSceneChanges(a_resourceManager);

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
		if (auto* _pDevTool = RefActiveScenePreview())
		{
			_pDevTool->DrawScenePreview();
			return;
		}

		// シーンの環境設定を流し込む。
		// 各シーンの描画より前に置くが、レンダーグラフが回るのはこの後なので順番はどちらでもよい
		ApplySceneAmbient();

		// すべてのシーンを描画
		for (auto& _scene : m_upBaseSceneVec)
		{
			// 命令のスタック
			_scene->Draw();
		}
	}

	void SceneManager::SetWorldFactory(WorldFactory a_factory)
	{
		m_worldFactory = std::move(a_factory);
	}

	std::unique_ptr<Engine::ECS::World> SceneManager::CreateWorld()
	{
		// ここで返せないとシーンがワールドを持てない。
		// 差し込みはアプリ起動時に一度きりなので、起動直後に気付ける
		ENGINE_ERRLOG(m_worldFactory != nullptr, "[SceneManager] ワールドの作り手が差し込まれていません");

		if (!m_worldFactory) return nullptr;

		return m_worldFactory();
	}

	//======================================================================================
	// 空のシーンを作る
	//======================================================================================
	Core::GUID SceneManager::CreateEmptyScene(Resource::AssetDatabase& a_assetDB, const std::string& a_path, const std::string& a_name)
	{
		if (a_name.empty())
		{
			ENGINE_WARNING("[Scene] 名前が空のためシーンを作成できません");
			return Core::GUID();
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
		const Core::GUID _checkGUID = a_assetDB.GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Core::DEFAULT_GUID)
		{
			ENGINE_WARNING("[Scene] すでに同じ名前のシーンがあります : %s", _basePath.c_str());
			return Core::GUID();
		}

		std::error_code _errorCode = {};
		std::filesystem::create_directories(_dirPath, _errorCode);
		if (_errorCode)
		{
			ENGINE_WARNING("[Scene] フォルダを作成できません : %s", _dirPath.c_str());
			return Core::GUID();
		}

		// アセットデータベースに場所を作る
		const Core::GUID _guid = a_assetDB.AddMetaData(_basePath, "Scene");

		//------------------------------------------------------------------
		// 空の中身を書き出す
		//
		// 開いているシーンには触らない。作るだけで、開くかどうかは呼び出し側が決める
		//------------------------------------------------------------------
		{
			BaseScene _emptyScene;
			_emptyScene.Enter();
			_emptyScene.SetGUID(_guid);

			Persistence::Archive _ar(Persistence::Archive::EMode::Save, _dirPath, a_name, "scene");
			_emptyScene.Archive(_ar);

			_emptyScene.Exit();
		}

		ENGINE_LOG("[Scene] 新規作成 : %s", _basePath.c_str());

		return _guid;
	}

	bool SceneManager::PushScene(Resource::ResourceManager& a_resourceManager, const Core::GUID& a_guid)
	{
		// シーンの新規作成 : GUIDからロードする
		auto _upScene = std::make_unique<BaseScene>();
		std::string _sceneFilePath = a_resourceManager.GetAssetDatabase().GetFilePathFromGUID(a_guid);
		if (_sceneFilePath.empty())
		{
			ENGINE_ERRLOG(false, "指定されたGUIDのシーンファイルが見つかりません");
			return false;
		}

		// どのシーンを読み込むかをログ出力する
		ENGINE_LOG("[Scene] ロード : %s", _sceneFilePath.c_str());

		// シーンの初期化
		_upScene->Enter();

		//------------------------------------------------------------------
		// 読み込み中のシーンを覚えておく
		//
		// このシーンがスタックへ乗るのは読み終えた後なので、
		// 読み込みの最中に RefWorld() を引かれると一つ前のシーン(または nullptr)が返る。
		// ワールドが無いと読めないリソース(プレハブ)がその隙に読まれると、
		// 空の実体がキャッシュに載ってしまうため、ここで行き先を教えておく
		//------------------------------------------------------------------
		m_pLoadingScene = _upScene.get();

		// シーンの再構築
		auto _fileDir = Core::File::GetDirFromPath(_sceneFilePath);
		auto _fileName = Core::File::GetFileNameWithoutExtension(_sceneFilePath);
		// 形式はビルドモード任せ(Auto)。Development までは .ojscene 優先、Shipping は .obscene のみ
		{
			Persistence::Archive _ar(Persistence::Archive::EMode::Load, _fileDir, _fileName, "scene");
			_upScene->Archive(_ar);
		}

		m_pLoadingScene = nullptr;

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
	void SceneManager::PopScene(Resource::ResourceManager& a_resourceManager)
	{
		if (m_upBaseSceneVec.empty()) return;

		// GPU待ち
		if (auto* _pGE = MainEngine::Instance().RefGraphicsEngine())
		{
			_pGE->RefRenderDevice()->WaitForFrame();

			//----------------------------------------------------------------------
			// 消えるワールドのカメラの実行インスタンスを捨てる
			//
			// 残しておくと、次のシーンが同じ描画構成を使うとき
			// (Desert_00 → Desert_02 など)に前のシーンの実行インスタンスが
			// 使い回されたり並んだりして、直接開いたときと違う状態で描かれる。
			// 間に別の描画構成のシーン(Home)を挟むと直るのはこのため。
			// GPUは上で待ったので、グラフのリソースはここで捨ててよい
			//----------------------------------------------------------------------
			if (auto* _pCameraPipelines = _pGE->RefCameraPipelines())
			{
				_pCameraPipelines->ReleaseWorldCameras(m_upBaseSceneVec.back()->RefWorld());
			}
		}

		// これを外すと1つも残らないか
		const bool _isLastScene = (m_upBaseSceneVec.size() == 1);

		m_upBaseSceneVec.back()->Exit();
		m_upBaseSceneVec.pop_back();

		//----------------------------------------------------------------------
		// 環境設定を流し込み直す
		//
		// 消えたシーンの空やフォグのノイズのテクスチャは、そのシーンが握っていた。
		// 次の描画まで待つと、下の SweepUnusedAll で捨てられたテクスチャの
		// ハンドルが SceneView に残ったままになるので、ここで残ったシーンのもの
		// (無ければ「無し」)へ差し替える
		//----------------------------------------------------------------------
		ApplySceneAmbient();

		// 後ろに何も残っていなければ、共有しているものをまとめて片付ける
		// (当たり判定の空間はワールドの持ち物なので、上の Exit で一緒に消えている)
		if (_isLastScene)
		{
			//--------------------------------------------------------------
			// 鳴り残っている音を止める
			//
			// 音を借りているものは、消えるときに自分で返す作りになっている
			//   ・コンポーネント … Release フェーズ(SoundFreeSystem など)
			//   ・UI / 進行役    … GameObjectManager の破棄が Release を呼ぶ
			// ただし返し漏れが1つでもあると、鳴っているボイスがプールに残り、
			// 次のシーンへ持ち越して鳴り続ける。ループ再生だと止まらない。
			//
			// シーンが1つも残っていないなら、鳴っていてよい音はもう無い。
			// ここで残りをまとめて止めて、取りこぼしを次のシーンへ持ち込まない。
			//
			// ※ 重ねたシーン(ポーズ)を外しただけのときは通らない。
			//    後ろのゲームで鳴っている音を巻き添えにしないため
			//--------------------------------------------------------------
			Audio::AudioManager::Instance().ReleaseInstances();

			// 誰も持っていないリソースはここで破棄する
			a_resourceManager.SweepUnusedAll();
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
	void SceneManager::ReplaceScene(Resource::ResourceManager& a_resourceManager, const Core::GUID& a_guid)
	{
		if (m_upBaseSceneVec.empty()) return;

		if (a_resourceManager.GetAssetDatabase().GetFilePathFromGUID(a_guid).empty())
		{
			ENGINE_WARNING("[Scene] 切り替え先のシーンが見つかりません : %s 今のシーンを続けます",
				a_guid.String().c_str());
			return;
		}

		PopScene(a_resourceManager);
		PushScene(a_resourceManager, a_guid);
	}

	Engine::ECS::World* SceneManager::RefWorld()
	{
		// 読み込み中のシーンがあるならそちらが「今のシーン」。
		// まだスタックに乗っていないので、ここで拾わないと引けない
		if (m_pLoadingScene) return m_pLoadingScene->RefWorld();

		if (m_upBaseSceneVec.empty()) return nullptr;

		return m_upBaseSceneVec.back()->RefWorld();
	}

	BaseScene* SceneManager::RefCurrentTopScene()
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

	BaseScene* SceneManager::RefAmbientSourceScene()
	{
		// 上から見て、最初に環境設定を使うシーン
		for (auto _it = m_upBaseSceneVec.rbegin(); _it != m_upBaseSceneVec.rend(); ++_it)
		{
			if ((*_it)->GetAmbient().IsEnabled()) return _it->get();
		}
		return nullptr;
	}

	//======================================================================================
	// シーンの環境設定を流し込む
	//--------------------------------------------------------------------------------------
	// 使うのは環境設定を使う一番上のシーンだけ。
	// ポーズ画面のように後ろの見た目をそのまま使いたいシーンは、環境設定を切っておけば
	// 後ろのゲームのものが使われ続ける。
	//
	// 1つも無ければ「無し」を流す(平行光の席もここで返す)。
	// 何も流さないと、前のシーンの空や太陽が残り続ける
	//======================================================================================
	void SceneManager::ApplySceneAmbient()
	{
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;

		if (BaseScene* _pScene = RefAmbientSourceScene())
		{
			_pScene->GetAmbient().Apply(*_pGE, m_ambientDLHandle);
		}
		else
		{
			SceneAmbient::ApplyNone(*_pGE, m_ambientDLHandle);
		}
	}

	void SceneManager::ReserveChangeScene(const Core::GUID& a_guid, const ESceneChangeType& a_changeType)
	{
		m_sceneChangeCmd.push({ a_guid,a_changeType });
	}

	void SceneManager::ApplyReservedSceneChanges(Resource::ResourceManager& a_resourceManager)
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
			if (auto* _pDevTool = MainEngine::Instance().RefDevTool()) _pDevTool->OnSceneChanged();

			switch (_cmd.changeType)
			{
			case ESceneChangeType::Push:
				PushScene(a_resourceManager, _cmd.sceneGUID);
				break;
			case ESceneChangeType::Pop:
				PopScene(a_resourceManager);
				break;
			case ESceneChangeType::Replace:
				ReplaceScene(a_resourceManager, _cmd.sceneGUID);
				break;
			case ESceneChangeType::Clear:
				// 1つずつ Pop に通す。最後の1つを外したところで
				// 共有の当たり判定空間が空になる
				while (!m_upBaseSceneVec.empty())
				{
					PopScene(a_resourceManager);
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