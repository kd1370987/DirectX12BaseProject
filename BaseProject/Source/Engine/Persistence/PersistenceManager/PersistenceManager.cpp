#include "PersistenceManager.h"

#include "Engine/Scene/SceneManager/SceneManager.h"

#include "../../ECS/World/World.h"
#include "../../../Application/Components/Persistence/GUIDComponent.h"
#include "../../../Application/Components/Transform/LocalTransformComponent.h"

namespace Engine::Persistence
{
	void PersistenceManager::SceneSave(ECS::World* a_pWorld, std::string a_path)
	{
		// Archiveクラスを「書き込みモード」で初期化
		auto _fileDir = FileUtility::GetDirFromPath(a_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_path);
		Archive _ar(Archive::Mode::Save, _fileDir, _fileName, "scene");

		// 保存するエンティティを収集
		std::vector<ECS::Entity> _entityVec = {};
		a_pWorld->ForEach<GUIDComponent>(
			[&_entityVec](ECS::ArchetypeChunk* a_pChunk, uint32_t a_count, GUIDComponent* a_guidArray)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					_entityVec.push_back(a_pChunk->entityData[_i]);
				}
			}
		);

		// 1. 全エンティティの配列管理を開始
		size_t _entityCount = _entityVec.size();
		if (_ar.BeginArray("Entities", _entityCount))
		{
			for (size_t _i = 0; _i < _entityCount; ++_i)
			{
				// 配列内の1要素（名前なしオブジェクト {} ）を開始
				if (_ar.BeginObject(_i))
				{
					const ECS::Entity& _entity = _entityVec[_i];
					ECS::Signature _sig = a_pWorld->GetSignature(_entity);

					// 2. このエンティティが持つコンポーネント名のリストを構築して保存
					std::vector<std::string> _compNames;
					for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
					{
						if (_sig.test(_typeID))
						{
							_compNames.push_back(_meta.name);
						}
					}
					_ar.VectorField("ComponentNames", _compNames);

					// 3. 各コンポーネントのデータを保存
					for (const std::string& _name : _compNames)
					{
						ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
						auto _func = a_pWorld->GetCompFunc(_typeID).archive;

						if (_func)
						{
							// 【超重要】コンポーネントごとにグループ（JSONのネスト構造）を作る
							// これをしないと、異なるコンポーネント間でフィールド名（"pos"など）が衝突します
							if (_ar.BeginGroup(_name))
							{
								_func(_ar, a_pWorld->NRefData(_entity, _typeID));
								_ar.EndGroup();
							}
						}
					}
					_ar.EndObject(); // エンティティオブジェクトの終了
				}
			}
			_ar.EndArray(); // エンティティ配列の終了
		}
	}
	void PersistenceManager::SceneLoad(ECS::World * a_pWorld, std::string a_path)
	{
		// Archiveクラスを「読み込みモード」で初期化
		auto _fileDir = FileUtility::GetDirFromPath(a_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_path);
		Archive _ar(Archive::Mode::Load, _fileDir, _fileName, "scene");

		// 【バグ修正】ロード時はファイルからサイズを読み込むため、0で初期化した変数オブジェクトを渡す
		size_t _entityCount = 0;

		// 配列の読み込みを開始（_entityCount にファイルから読み込まれた個数が書き込まれる）
		if (_ar.BeginArray("Entities", _entityCount))
		{
			for (size_t _i = 0; _i < _entityCount; ++_i)
			{
				// 配列内の各エンティティオブジェクトの読み込み開始
				if (_ar.BeginObject(_i))
				{
					// まずコンポーネント名のリストを読み込む
					std::vector<std::string> _compNames;
					_ar.VectorField("ComponentNames", _compNames);

					// 2. リストからシグネチャを構築
					ECS::Signature _sig = {};
					for (const std::string& _name : _compNames)
					{
						ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
						if (_typeID != ECS::Limits::INVALID_COMPONENTTYPEID)
						{
							_sig.set(_typeID);
						}
					}

					// 3. 構築したシグネチャを元にエンティティを生成
					ECS::Entity _entity = a_pWorld->CreateEntity(_sig);

					// 4. 各コンポーネントのデータを読み込んでメモリに流し込む
					for (const std::string& _name : _compNames)
					{
						ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
						if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

						auto _func = a_pWorld->GetCompFunc(_typeID).archive;
						if (_func)
						{
							// セーブ時と同じコンポーネント名のグループに入り、データを解析
							if (_ar.BeginGroup(_name))
							{
								void* _data = a_pWorld->NRefData(_entity, _typeID);
								_func(_ar, _data);
								_ar.EndGroup();
							}
						}
					}
					_ar.EndObject(); // エンティティオブジェクトの終了
				}
			}
			_ar.EndArray(); // エンティティ配列の終了
		}
		
	}
}