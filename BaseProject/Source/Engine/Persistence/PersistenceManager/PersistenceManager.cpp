#include "PersistenceManager.h"

#include "../../../Application/Scene/SceneManager.h"

#include "../../ECS/World/World.h"
#include "../../../Application/Components/Persistence/GUIDComponent.h"
#include "../../../Application/Components/Transform/TRSComponent.h"

namespace Engine::Persistence
{
	void PersistenceManager::SeceneSerialize(std::string a_path)
	{
		// シーン単位での保存

		// 保存するエンティティを収集
		std::vector<ECS::Entity> _entityVec = {};

		// ワールド取得
		auto _pWorld = SceneManager::Instance().RefWorld();

		// GUIDを保有しているオブジェクトのみ収集
		//_pWorld->ForEach<GUIDComponent>(
		_pWorld->ForEach<TRSComponent>(
			[&_pWorld,&_entityVec]
			(
				ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				TRSComponent* a_guidArray
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
			nlohmann::json _json;
			
			// エンティティが持つコンポーネントを記録
			ECS::Signature _sig = _pWorld->GetSignature(_entity);
			for (auto& [_typeID,_meta] : _pWorld->GetAllComponentMetaData())
			{
				if (!_sig.test(_typeID)) continue;

				// シリアライズ関数があれば実行
				auto _func = _pWorld->GetCompFunc(_typeID).serialize;
				if(_func)
				{
					_func(_pWorld->NRefData(_entity, _typeID), _json);
				}
			}


			// エンティティの配列に追加
			_jsonArray.push_back(_json);
		}

		// ファイルを開く
		std::ofstream _ofs(a_path);
		if (!_ofs)
		{
			// 失敗したら
			return;
		}

		_ofs << _jsonArray.dump(4);
	}
}