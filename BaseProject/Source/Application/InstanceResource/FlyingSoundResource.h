#pragma once

#include "../../Engine/Resource/Data/Sound/Sound.h"

//==========================================================================================
//
// 飛翔音のボイスをエンティティ単位で預かるワールドリソース。
//
// なぜコンポーネントではなくここで持つのか:
//   弾やミサイルは AddRemoveEntity で消えるが、この経路では Release フェーズが走らない
//   (走るのは ReleaseTag が付いたエンティティ = シーン終了時とエディタのリフレッシュのみ)。
//   つまりコンポーネントにサウンドインスタンスのハンドルを持たせると、
//   エンティティが消えた瞬間に返却する機会が無くなり、ループ再生のボイスが
//   プールに残って鳴り続けてしまう。
//
//   そこで「毎フレーム、生きているエンティティを見て印を付ける
//   → 付かなかったものは消えたとみなして回収する」形にしている。
//   キーは世代込みの Entity なので、添え字が別のエンティティに再利用されても
//   別物として扱われ、取り違えない。
//
// 産む/回収する : FlyingSoundSystem(PostUpdate)
//
//==========================================================================================

// エンティティ1体分の鳴っているボイス
struct FlyingSoundVoice
{
	Engine::Handle<Engine::Resource::SoundInstance> handle = {};	// 鳴らしているインスタンス
	bool isAlive = false;											// 今フレーム持ち主を見かけたか
};

struct FlyingSoundResource
{
	// 持ち主のエンティティ -> ボイス
	std::unordered_map<Engine::ECS::Entity, FlyingSoundVoice> voiceMap = {};

	// フレーム頭に呼ぶ。全部「見かけていない」に戻す
	void ResetAliveFlags()
	{
		for (auto& [_entity, _voice] : voiceMap)
		{
			_voice.isAlive = false;
		}
	}

	// 持ち主のボイスを引く(無ければ nullptr)
	FlyingSoundVoice* Find(const Engine::ECS::Entity& a_entity)
	{
		auto _it = voiceMap.find(a_entity);
		if (_it == voiceMap.end()) return nullptr;
		return &_it->second;
	}

	// ボイスを預ける
	void Add(const Engine::ECS::Entity& a_entity, const Engine::Handle<Engine::Resource::SoundInstance>& a_handle)
	{
		FlyingSoundVoice _voice = {};
		_voice.handle  = a_handle;
		_voice.isAlive = true;
		voiceMap[a_entity] = _voice;
	}
};
