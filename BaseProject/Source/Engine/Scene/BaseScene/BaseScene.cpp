#include "BaseScene.h"

#include "Engine/ECS/World/World.h"									// ECS
#include "Engine/Scene/SceneManager/SceneManager.h"					// シーンマネージャー

// コンポーネント
#include "Application/Components/Persistence/GUIDComponent.h"

// エンジン系
#include "../../MainEngine.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Collision/CollisionWorld.h"
#include "../../Input/InputManager/InputManager.h"
#include "../../Editor/Editor.h"
#include "../../Raytracing/RaytracingEngine/RaytracingEngine.h"
#include "../../GameObject/GameObjectManager/GameObjectManager.h"

namespace Engine::Scene
{
	BaseScene::BaseScene()
	{}

	BaseScene::~BaseScene()
	{}

	void BaseScene::Enter()
	{
		// ワールド作成
		m_upWorld = std::make_unique<Engine::ECS::World>();
		m_upWorld->Init();

		// アプリ寿命のサービスを差し込む。
		// シングルトンを名指しするのはここ(合成の入り口)だけにして、
		// 各システムは SystemContext 経由で受け取る。
		Engine::ECS::EngineServices _services = {};
		_services.pMainEngine		= &Engine::MainEngine::Instance();
		_services.pResourceManager	= &Engine::Resource::ResourceManager::Instance();
		_services.pInputManager		= &Engine::Input::InputManager::Instance();
		_services.pMainEditor		= &Engine::Editor::MainEditor::Instance();
		_services.pRayEngine		= &Engine::Raytracing::RayEngine::Instance();
		m_upWorld->SetEngineServices(_services);

		// ワールド設定の呼びだし
		SceneManager::Instance().InvokeWorldInitCallback(m_upWorld.get());

		// ECS外オブジェクトの生成
		m_upGameObjectManager = std::make_unique<GameObject::GameObjectManager>();
	}

	void BaseScene::Exit()
	{
		m_upWorld->Release();
		Engine::MainEngine::Instance().RefCollisionWorld()->Clear();
	}

	void BaseScene::Update(float a_dt)
	{

		// シーンの初めに一括でエンティティを生成・削除
		// 解放処理と初期化処理も含まれているため、呼び出しはシングルスレッド限定
		m_upWorld->BegineFrame();
		static int _i = 0;
		//_i++;
		if(_i == 10)
		{
			// すべてのECS参照カウントをリセット
			Engine::Resource::ResourceManager::Instance().AllResetECSRefs();

			// ECSカウントの収集
			m_upWorld->RunSystem(Engine::ECS::ESystemType::GC, a_dt);

			// 参照カウントがなくなった場合リソースの解放をする
			Engine::Resource::ResourceManager::Instance().RunGarbageCollectionSweep();

			_i = 0;
		}
		
		// シーンのシステム処理
		m_upWorld->RunSystem(Engine::ECS::ESystemType::Input, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PreUpdate, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Update, a_dt);

		// 動的コライダーの submit（Update まで）が終わったこのタイミングで動的TLASを構築する。
		// Physics フェーズの判定クエリが最新の動的ワールドを参照できるようにするため、
		// 必ず Physics の前に置くこと。
		if (auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld())
		{
			_pCollWorld->BuildDynamicWorld();
		}

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Physics, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Animation, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Camera, a_dt);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PostUpdate, a_dt);
	}

	void BaseScene::Draw()
	{
		m_upWorld->RunSystem(Engine::ECS::ESystemType::PreDraw, 0.0f);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::Draw, 0.0f);

		m_upWorld->RunSystem(Engine::ECS::ESystemType::PostDraw, 0.0f);
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
	}
}