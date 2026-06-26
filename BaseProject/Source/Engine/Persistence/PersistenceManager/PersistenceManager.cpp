#include "PersistenceManager.h"

#include "../../../Application/Scene/SceneManager.h"

#include "../../ECS/World/World.h"
#include "../../../Application/Components/Persistence/GUIDComponent.h"
#include "../../../Application/Components/Transform/LocalTransformComponent.h"

namespace Engine::Persistence
{
	void PersistenceManager::SeceneSerialize(ECS::World* a_pWorld, std::string a_path)
	{
		// シーン単位での保存

		// 保存するエンティティを収集
		std::vector<ECS::Entity> _entityVec = {};

		// GUIDを保有しているオブジェクトのみ収集
		//_pWorld->ForEach<GUIDComponent>(
		a_pWorld->ForEach<LocalTransformComponent>(
			[&a_pWorld,&_entityVec]
			(
				ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				LocalTransformComponent* a_guidArray
			) 
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					ECS::Entity _entity = a_pChunk->entityData[_i];
					_entityVec.push_back(_entity);
				}
			}
		);
		// 保存する
		nlohmann::json _jsonArray = nlohmann::json::array();
		for (const ECS::Entity& _entity : _entityVec)
		{
			// エンティティ一つ
			nlohmann::json _entityJson = nlohmann::json::array();
			
			// エンティティが持つコンポーネントを記録
			ECS::Signature _sig = a_pWorld->GetSignature(_entity);
			for (auto& [_typeID,_meta] : a_pWorld->GetAllComponentMetaData())
			{
				if (!_sig.test(_typeID)) continue;

				// コンポーネント名を記録
				nlohmann::json _compJson;
				_compJson["CompName"] = _meta.name;

				// シリアライズ関数があれば実行
				auto _func = a_pWorld->GetCompFunc(_typeID).serialize;
				if(_func)
				{
					_func(a_pWorld->NRefData(_entity, _typeID), _compJson);
				}

				// エンティティに追加
				_entityJson.push_back(_compJson);
			}


			// エンティティの配列に追加
			_jsonArray.push_back(_entityJson);
		}

		// ファイルを開く
		std::ofstream _ofs(a_path);
		if (!_ofs)
		{
			// 失敗したら
			return;
		}

		_ofs << _jsonArray.dump(4);
		_ofs.flush();
		_ofs.close();
	}
	void PersistenceManager::SeceneDeserialize(ECS::World* a_pWorld, std::string a_path)
	{
		// ファイル読み込み
		std::ifstream _ifs(a_path);
		if (!_ifs) return;

		nlohmann::json _jsonArray;
		_ifs >> _jsonArray;

		// 全エンティティ復元
		_jsonArray.size();

		for (auto& _entityJson : _jsonArray)
		{
			// シグネチャ作成
			ECS::Signature _sig = {};
			for (auto& _compJson : _entityJson)
			{
				// コンポーネントID取得
				std::string _name = _compJson["CompName"];
				ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
				if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

				// シグネチャにセット
				_sig.set(_typeID);
			}

			// エンティティ生成
			ECS::Entity _entity = a_pWorld->CreateEntity(_sig);

			// データの書き込み
			for (auto& _compJson : _entityJson)
			{
				// コンポーネントID取得
				std::string _name = _compJson["CompName"];
				ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
				if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

				auto _func = a_pWorld->GetCompFunc(_typeID).deserialize;
				if (_func)
				{
					void* _data = a_pWorld->NRefData(_entity,_typeID);
					_func(_data,_compJson);
				}
			}
		}
	}
	
	
}