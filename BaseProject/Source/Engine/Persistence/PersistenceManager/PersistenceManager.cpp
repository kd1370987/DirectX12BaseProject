#include "PersistenceManager.h"

#include "Engine/Scene/SceneManager/SceneManager.h"

#include "../../ECS/World/World.h"
#include "../../../Application/Components/Persistence/GUIDComponent.h"
#include "../../../Application/Components/Transform/LocalTransformComponent.h"

namespace Engine::Persistence
{
	void PersistenceManager::SceneSave(ECS::World* a_pWorld, std::string a_path)
	{
		auto _fileDir = FileUtility::GetDirFromPath(a_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_path);
		Archive _ar(Archive::Mode::Save, _fileDir, _fileName, "scene");

		SceneArchive(a_pWorld, _ar);
	}
	void PersistenceManager::SceneLoad(ECS::World * a_pWorld, std::string a_path)
	{
		auto _fileDir = FileUtility::GetDirFromPath(a_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_path);
		Archive _ar(Archive::Mode::Load, _fileDir, _fileName, "scene");

		SceneArchive(a_pWorld, _ar);
	}

	void PersistenceManager::SceneArchive(ECS::World* a_pWorld, Archive& a_ar)
	{
		size_t _entityCount = 0;
		std::vector<ECS::Entity> _entityVec = {};

		// ---------------------------------------------------------
		// 1. セーブ時のみ：保存対象のエンティティを事前収集
		// ---------------------------------------------------------
		if (a_ar.GetMode() == Archive::Mode::Save) // ※実装に合わせてIsSaving()等に変更してください
		{
			a_pWorld->ForEach<GUIDComponent>(
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
		// 2. 配列の処理（セーブ時はサイズを書き込み、ロード時は読み込んで_entityCountに入る）
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
					if (a_ar.GetMode() == Archive::Mode::Save)
					{
						_entity = _entityVec[_i];
						ECS::Signature _sig = a_pWorld->GetSignature(_entity);

						for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
						{
							if (_sig.test(_typeID))
							{
								_compNames.push_back(_meta.name);
							}
						}
					}

					// 【共通処理】コンポーネント名のリストを保存 or 読み込み
					// セーブなら書き込まれ、ロードなら _compNames にデータが入る
					a_ar.VectorField("ComponentNames", _compNames);

					// 【ロード時のみ】読み込んだリストからシグネチャを作り、エンティティを生成
					if (a_ar.GetMode() == Archive::Mode::Load)
					{
						ECS::Signature _sig = {};
						for (const std::string& _name : _compNames)
						{
							ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
							if (_typeID != ECS::Limits::INVALID_COMPONENTTYPEID)
							{
								_sig.set(_typeID);
							}
						}
						_entity = a_pWorld->CreateEntity(_sig);
					}

					// ---------------------------------------------------------
					// 3. 【共通処理】各コンポーネントデータのシリアライズ
					// ---------------------------------------------------------
					for (const std::string& _name : _compNames)
					{
						ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
						if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

						auto _func = a_pWorld->GetCompFunc(_typeID).archive;
						if (_func)
						{
							// セーブもロードも同じグループ構造で実行
							if (a_ar.BeginGroup(_name))
							{
								void* _data = a_pWorld->NRefData(_entity, _typeID);
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