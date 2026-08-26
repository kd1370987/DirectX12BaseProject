#include "BaseScene.h"

#include "Engine/ECS/World/World.h"									// ECS
#include "Engine/Scene/SceneManager/SceneManager.h"					// シーンマネージャー

// コンポーネント
#include "Application/Components/Persistence/GUIDComponent.h"

// エンジン系
#include "../../MainEngine.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../Option/OptionManager.h"
#include "../../Collision/CollisionWorld.h"
#include "../../Input/InputManager/InputManager.h"
#include "../../Editor/Editor.h"
#include "../../Raytracing/RaytracingEngine/RaytracingEngine.h"
#include "../../Audio/AudioManager.h"
#include "../../GameObject/GameObjectManager/GameObjectManager.h"

// アプリ側UIオブジェクト
#include "Application/Object/UI/CombatReticleHUD/CombatReticleHUD.h"


namespace Engine::Scene
{
	BaseScene::BaseScene()
	{}

	BaseScene::~BaseScene()
	{}

	//======================================================================================
	// シーン用ワールドの生成
	//--------------------------------------------------------------------------------------
	// 通常のシーンとエディターのプレビュー用シーンで同じものを使う。詳細はヘッダを参照。
	//======================================================================================
	std::unique_ptr<Engine::ECS::World> CreateSceneWorld()
	{
		// 実体を作るのは上位層(App::ECS::World)。
		// エンジンは基盤の Engine::ECS::World としてしか触らない
		auto _upWorld = SceneManager::Instance().CreateWorld();
		if (!_upWorld) return nullptr;

		_upWorld->Init();

		// アプリ寿命のサービスを差し込む。
		// シングルトンを名指しするのはここ(合成の入り口)だけにして、
		// 各システムは SystemContext 経由で受け取る。
		Engine::ECS::EngineServices _services = {};
		_services.pMainEngine		= &Engine::MainEngine::Instance();
		_services.pResourceManager	= &Engine::Resource::ResourceManager::Instance();
		_services.pAssetDatabase	= &Engine::Resource::AssetDatabase::Instance();
		_services.pInputManager		= &Engine::Input::InputManager::Instance();
		_services.pMainEditor		= &Engine::Editor::MainEditor::Instance();
		_services.pRayEngine		= &Engine::Raytracing::RayEngine::Instance();
		_services.pAudioManager		= &Engine::Audio::AudioManager::Instance();
		_services.pJobSystem		= Engine::MainEngine::Instance().RefJobSystem();
		_services.pOptionManager	= &Engine::Option::OptionManager::GetInstance();
		_upWorld->SetEngineServices(_services);

		// 当たり判定の空間。
		//
		// シーン(ワールド)ごとに1つ持つ。以前はエンジンが1つだけ持つ共有物だったため、
		//   ・ポーズ画面を重ねただけで消すと後ろのゲームの静的コライダーが失われる
		//     (登録は Start の一度きりなので戻らない)
		//   ・エフェクトエディターのプレビューがゲームのコライダーと同じ空間に乗る
		// といった具合に、持ち主が誰なのかを場所ごとに考える必要があった。
		// ワールドと同じ寿命にしておけば、シーンを消せば当たり判定も一緒に消える。
		//
		// ここで足しているのでプレビュー用のワールドにも必ず1つある。
		// システムは a_ctx.pWorld->GetResource<CollisionWorld>() で引くこと。
		_upWorld->AddResource<Collision::CollisionWorld>();

		// ゲーム固有のコンポーネントとシステムの登録。
		// 何を登録するかはワールドの実体(派生)が持っている
		_upWorld->RegisterGameTypes();

		return _upWorld;
	}

	void BaseScene::Enter()
	{
		// ワールド作成
		m_upWorld = CreateSceneWorld();

		// ECS外オブジェクトの生成
		// 中身はシーン読み込み(Archive)またはエディターの AddObject で追加される。
		// 自シーンのワールドを渡し、各オブジェクトへは ObjectContext 経由で配らせる。
		m_upGameObjectManager = std::make_unique<GameObject::GameObjectManager>(m_upWorld.get());
	}

	//======================================================================================
	// 解放
	//--------------------------------------------------------------------------------------
	// エンティティを消すところまで。コンポーネントが借りているリソースは
	// 解放フック(ComponentTraits<T>::Release)が返すので、ここで数え直すことはしない。
	//
	// 当たり判定の空間はワールドの持ち物なので、ワールドと一緒に消える。
	// 残るのは「誰も持っていないリソースの破棄」だけで、これは他にシーンが
	// 残っているかを見ないと決められないので呼び出し側(SceneManager::PopScene)が持つ。
	//======================================================================================
	void BaseScene::Exit()
	{
		m_upWorld->Release();
	}

	void BaseScene::Update(float a_dt)
	{

		m_upGameObjectManager->PreUpdate();

		// シーンの初めに一括でエンティティを生成・削除
		// 解放処理と初期化処理も含まれているため、呼び出しはシングルスレッド限定
		// (この中で Start フェーズが走り、静的コライダーの登録もここで行われる)
		m_upWorld->BeginFrame();

		// このシーンの当たり判定の空間。ワールドの持ち物なので、
		// 重ねているシーンがあっても互いのコライダーが混ざることはない
		auto& _collWorld = m_upWorld->GetResource<Engine::Collision::CollisionWorld>();

		// 動的ワールドは毎フレーム詰めなおす。
		// この後の Update フェーズ(SubmitDynamicColliderSystem)が積み直すので、
		// 積む前に空にしておくこと
		_collWorld.ClearDynamicWorld(kDynamicColliderReserve);

		// シーンのシステム処理
		//
		// 入力フェーズはモードに関わらず毎フレーム回す。
		// プレイモード以外では InputManager が無入力を返すので、入力フェーズの
		// システムが MoveIntent などへ 0 を書き込み続けることになる。
		// ここを止めてしまうと最後に書き込まれた入力がそのまま残り、
		// エディターへ戻ってもプレイヤーが走り続ける(＝入力が残る)。
		m_upWorld->RunSystem(Engine::ECS::ESystemType::Input, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PreUpdate, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Update, a_dt);

		// 動的コライダーの submit（Update まで）が終わったこのタイミングでTLASを構築する。
		// Physics フェーズの判定クエリが最新のワールドを参照できるようにするため、
		// 必ず Physics の前に置くこと。
		//
		// 静的側もここで構築する。登録は上の BeginFrame(Start フェーズ)で済んでいるので、
		// 置いたそのフレームから判定に乗る。
		// (以前は全シーンの更新が終わった後=BeginDraw で構築していたため、
		//  静的コライダーが判定へ反映されるのが1フレーム遅れていた)
		// 中身に変更が無ければ ReBuildStaticTLAS は素通りするので、毎フレーム呼んでよい
		_collWorld.BuildDynamicWorld();
		_collWorld.BuildWorld();

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Physics, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Animation, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Camera, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PostUpdate, a_dt);

		m_upGameObjectManager->Update(a_dt);
	}

	void BaseScene::Draw()
	{
		// 静的コライダーのAABBをデバッグ表示へ積む。
		// 実際に出すかどうかは MainEditor 側(デバッグ表示の設定)が決める
		m_upWorld->GetResource<Engine::Collision::CollisionWorld>().DrawDebug();

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PreDraw, 0.0f);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Draw, 0.0f);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PostDraw, 0.0f);

		m_upGameObjectManager->Draw(0.0f);
	}

	void BaseScene::Archive(Persistence::Archive& a_ar)
	{
		size_t _entityCount = 0;
		std::vector<ECS::Entity> _entityVec = {};

		// ---------------------------------------------------------
		// セーブ時のみ：保存対象のエンティティを事前収集
		// ---------------------------------------------------------
		if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
		{
			m_upWorld->ForEach<GUIDComponent>(
				[&_entityVec](ECS::ArchetypeChunk* a_pChunk, uint32_t a_count, GUIDComponent* a_guidArray)
				{
					for (size_t _i = 0; _i < a_count; ++_i)
					{
						_entityVec.push_back(a_pChunk->entityData[_i]);
					}
				}
			);
			_entityCount = _entityVec.size();
		}

		// ---------------------------------------------------------
		// 配列の処理（セーブ時はサイズを書き込み、ロード時は読み込んで_entityCountに入る）
		// ---------------------------------------------------------
		if (a_ar.BeginArray("Entities", _entityCount))
		{
			for (size_t _i = 0; _i < _entityCount; ++_i)
			{
				if (a_ar.BeginObject(_i))
				{
					std::vector<std::string> _compNames;
					ECS::Entity _entity;

					// 【セーブ時のみ】エンティティからコンポーネント名リストを作成
					if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
					{
						_entity = _entityVec[_i];
						ECS::Signature _sig = m_upWorld->GetSignature(_entity);

						for (auto& [_typeID, _meta] : m_upWorld->GetAllComponentMetaData())
						{
							if (_sig.test(_typeID))
							{
								_compNames.push_back(_meta.name);
							}
						}
					}

					// コンポーネント名のリストを保存 or 読み込み
					// セーブなら書き込まれ、ロードなら _compNames にデータが入る
					a_ar.VectorField("ComponentNames", _compNames);

					// 【ロード時のみ】読み込んだリストからシグネチャを作り、エンティティを生成
					if (a_ar.GetMode() == Persistence::Archive::Mode::Load)
					{
						ECS::Signature _sig = {};
						for (const std::string& _name : _compNames)
						{
							ECS::ComponentTypeID _typeID = m_upWorld->GetCompTypeID(_name);
							if (_typeID != ECS::Limits::INVALID_COMPONENTTYPEID)
							{
								_sig.set(_typeID);
							}
						}
						_entity = m_upWorld->CreateEntity(_sig);
					}

					// ---------------------------------------------------------
					// 各コンポーネントデータのシリアライズ
					// ---------------------------------------------------------
					for (const std::string& _name : _compNames)
					{
						ECS::ComponentTypeID _typeID = m_upWorld->GetCompTypeID(_name);
						if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

						auto _func = m_upWorld->GetCompFunc(_typeID).archive;
						if (_func)
						{
							// セーブもロードも同じグループ構造で実行
							if (a_ar.BeginGroup(_name))
							{
								void* _data = m_upWorld->NRefData(_entity, _typeID);
								_func(a_ar, _data);
								a_ar.EndGroup();
							}
						}
					}
					a_ar.EndObject(); // エンティティオブジェクトの終了
				}
			}
			a_ar.EndArray(); // エンティティ配列の終了
		}

		// ---------------------------------------------------------
		// ECS外オブジェクト(GameObject)のシリアライズ
		// タイプインデックス / GUID / データ を GameObjectManager 側で処理する。
		// ---------------------------------------------------------
		if (m_upGameObjectManager)
		{
			m_upGameObjectManager->Archive(a_ar);
		}
	}
}