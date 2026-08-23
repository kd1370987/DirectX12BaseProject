#pragma once

#include "../../Engine/Resource/Data/Sound/Sound.h"
#include "../../Engine/Audio/AudioManager.h"

//==========================================================================================
//
// 飛翔音のボイスをエンティティ単位で預かるワールドリソース。
//
// なぜコンポーネントではなくここで持つのか:
//   自滅するエンティティ(寿命切れ・着弾)が Release フェーズを通らなかった頃の名残り。
//   当時はコンポーネントにハンドルを持たせると返却する機会が無く、
//   ループ再生のボイスがプールに残って鳴り続けてしまった。
//   そこで「毎フレーム、生きているエンティティを見て印を付ける
//   → 付かなかったものは消えたとみなして回収する」形にしている。
//   キーは世代込みの Entity なので、添え字が別のエンティティに再利用されても
//   別物として扱われ、取り違えない。
//
//   現在は自滅も AddReleaseEntity(ReleaseTag経由)になったので、
//   他のサウンドと同じくコンポーネントにハンドルを持たせて
//   Release フェーズで返す形にもできる。動いているものを触る必要が出たら寄せること。
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

	//----------------------------------------------------------------------------------
	// ワールドと一緒に消えるときに、抱えているボイスを全部返す
	//
	// 回収は FlyingSoundSystem の「見かけなかったものを消す」掃除に任せているが、
	// あれが回るのはシーンが動いている間だけ。シーンを切り替えるとワールドごと
	// 消えるので掃除が走らず、ループ再生のボイスがプールに残って鳴り続けていた。
	//
	// ここで AudioManager を名指ししているのは、デストラクタには
	// コンテキストを渡す口が無いため。
	// シーンの破棄(SceneManager::Release)はオーディオの解放より先に走るので、
	// この時点では AudioManager は生きている
	//----------------------------------------------------------------------------------
	~FlyingSoundResource()
	{
		ReleaseAll();
	}

	// 抱えているボイスを全部返す(鳴っていても止まる)
	void ReleaseAll()
	{
		auto& _audioManager = Engine::Audio::AudioManager::Instance();

		for (auto& [_entity, _voice] : voiceMap)
		{
			_audioManager.ReleaseSoundInstance(_voice.handle);
		}

		voiceMap.clear();
	}

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
