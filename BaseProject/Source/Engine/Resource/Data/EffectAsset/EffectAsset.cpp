#include "EffectAsset.h"

#include "../../Manager/ResourceManager/ResourceManager.h"
#include "../../../Audio/AudioManager.h"

namespace Engine::Resource
{
	const char* ToString(EEffectSpace a_space)
	{
		switch (a_space)
		{
		case EEffectSpace::LocalOffset:		return "LocalOffset";
		case EEffectSpace::WorldMatrix:		return "WorldMatrix";
		case EEffectSpace::ReverseVelocity:	return "ReverseVelocity";
		default:							return "Unknown";
		}
	}

	//======================================================================================
	// EffectTiming
	//======================================================================================
	void EffectTiming::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("StartDelay", startDelay);
		a_ar.Field("Duration", duration);
	}

	//======================================================================================
	// EffectParticlePart
	//======================================================================================
	void EffectParticlePart::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("ParticleGUID", particleGUID);

		a_ar.Field("Space", space);
		a_ar.Field("PosOffset", posOffset);
		a_ar.Field("EmitDir", emitDir);

		a_ar.Field("EmitCount", emitCount);
		a_ar.Field("EmitRate", emitRate);

		timing.Archive(a_ar);

		a_ar.Field("BaseScale", baseScale);
		a_ar.Field("MinScale", minScale);
		a_ar.Field("MaxScale", maxScale);
		a_ar.Field("PositionRadius", positionRadius);
		a_ar.Field("DirectionAngle", directionAngle);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		a_ar.Field("EmitShape", emitShape);
	}

	//======================================================================================
	// EffectMeshPart
	//======================================================================================
	void EffectMeshPart::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("ModelGUID", modelGUID);

		a_ar.Field("PosOffset", posOffset);
		a_ar.Field("Rotation", rotation);
		a_ar.Field("Scale", scale);

		timing.Archive(a_ar);

		a_ar.Field("ColorScale", colorScale);
		a_ar.Field("EmissiveColor", emissiveColor);
		a_ar.Field("EmissiveIntensity", emissiveIntensity);

		a_ar.Field("EndScale", endScale);
		a_ar.Field("EndAlpha", endAlpha);
		a_ar.Field("EndEmissiveIntensity", endEmissiveIntensity);
	}

	//======================================================================================
	// EffectSoundPart
	//======================================================================================
	void EffectSoundPart::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("SoundGUID", soundGUID);

		timing.Archive(a_ar);

		a_ar.Field("Vol", vol);
		a_ar.Field("IsLoop", isLoop);
		a_ar.Field("Is3DSound", is3DSound);
		a_ar.Field("DistanceScaler", distanceScaler);
		a_ar.Field("IsWaitFinish", isWaitFinish);
	}

	//======================================================================================
	// EffectInstance : 借りている声への操作
	//
	// どれもアセットを見ないので、アセットが読めていなくても呼べる。
	// 解放の経路(エンティティが消える)はアセットの生存と無関係に走るため
	//======================================================================================
	void EffectInstance::SetSoundPos(Engine::Audio::AudioManager& a_audioManager, const Math::Vector3& a_pos)
	{
		soundPos = a_pos;

		for (size_t _i = 0; _i < EFFECT_SOUND_MAX; ++_i)
		{
			auto* _pInstance = a_audioManager.RefInstance(soundHandles[_i]);
			if (!_pInstance) continue;

			// 2Dで発行されたインスタンスに位置を渡すと DirectXTK が例外を投げる
			if (!_pInstance->Is3D()) continue;

			_pInstance->SetPos(soundPos);
		}
	}

	void EffectInstance::ReleaseSounds(Engine::Audio::AudioManager& a_audioManager)
	{
		for (size_t _i = 0; _i < EFFECT_SOUND_MAX; ++_i)
		{
			// 鳴っていても止めて返却される
			a_audioManager.ReleaseSoundInstance(soundHandles[_i]);
			soundHandles[_i] = {};
			soundTriggered[_i] = false;

			// 何から発行したかも空にする。
			// 残しておくと、次に作り直すときに「合っている」と判断されて声が湧かない
			soundSourceGUID[_i] = Engine::DefaultGUID;
			soundSource3D[_i] = false;
		}
	}

	//======================================================================================
	// EffectAsset : 編集
	//======================================================================================
	bool EffectAsset::AddParticlePart()
	{
		if (m_particleParts.size() >= EFFECT_PARTICLE_MAX) return false;
		m_particleParts.emplace_back();
		return true;
	}

	bool EffectAsset::AddMeshPart()
	{
		if (m_meshParts.size() >= EFFECT_MESH_MAX) return false;
		m_meshParts.emplace_back();
		return true;
	}

	bool EffectAsset::AddSoundPart()
	{
		if (m_soundParts.size() >= EFFECT_SOUND_MAX) return false;
		m_soundParts.emplace_back();
		return true;
	}

	void EffectAsset::RemoveParticlePart(size_t a_index)
	{
		if (a_index >= m_particleParts.size()) return;
		m_particleParts.erase(m_particleParts.begin() + a_index);
	}

	void EffectAsset::RemoveMeshPart(size_t a_index)
	{
		if (a_index >= m_meshParts.size()) return;
		m_meshParts.erase(m_meshParts.begin() + a_index);
	}

	void EffectAsset::RemoveSoundPart(size_t a_index)
	{
		if (a_index >= m_soundParts.size()) return;
		m_soundParts.erase(m_soundParts.begin() + a_index);
	}

	//======================================================================================
	// EffectAsset : 保存・読み込み
	//======================================================================================
	void EffectAsset::Archive(Persistence::Archive& a_ar)
	{
		a_ar.StringField("Name", m_name);

		//------------------------------------------------------------------
		// パーティクル
		//
		// 個数から書くので、パーツを増やしても既存のファイルは読める
		// (中身の並びを変えた場合はバイナリが崩れるので作り直しが要る)
		//------------------------------------------------------------------
		size_t _particleCount = m_particleParts.size();
		if (a_ar.BeginArray("ParticleParts", _particleCount))
		{
			// ファイルに入っている数はそのまま読み切る。
			// 上限超過をここで打ち切るとバイナリの読み位置がずれて、
			// 後ろに続くメッシュパーツまで壊れる
			if (a_ar.IsLoading())
			{
				m_particleParts.assign(_particleCount, EffectParticlePart{});
			}

			for (size_t _i = 0; _i < _particleCount; ++_i)
			{
				if (a_ar.BeginObject(_i))
				{
					m_particleParts[_i].Archive(a_ar);
					a_ar.EndObject();
				}
			}
			a_ar.EndArray();

			// 読み切ってから切り捨てる。
			// 実体側(EffectInstance)の進行状態が固定長なので、上限を超えたぶんは動かせない
			if (a_ar.IsLoading() && m_particleParts.size() > EFFECT_PARTICLE_MAX)
			{
				m_particleParts.resize(EFFECT_PARTICLE_MAX);
			}
		}

		//------------------------------------------------------------------
		// メッシュ
		//------------------------------------------------------------------
		size_t _meshCount = m_meshParts.size();
		if (a_ar.BeginArray("MeshParts", _meshCount))
		{
			if (a_ar.IsLoading())
			{
				m_meshParts.assign(_meshCount, EffectMeshPart{});
			}

			for (size_t _i = 0; _i < _meshCount; ++_i)
			{
				if (a_ar.BeginObject(_i))
				{
					m_meshParts[_i].Archive(a_ar);
					a_ar.EndObject();
				}
			}
			a_ar.EndArray();

			if (a_ar.IsLoading() && m_meshParts.size() > EFFECT_MESH_MAX)
			{
				m_meshParts.resize(EFFECT_MESH_MAX);
			}
		}

		//------------------------------------------------------------------
		// サウンド
		//
		// 後から足した並びなので、必ず末尾に置くこと。
		// JSON は「SoundParts が無ければ音なし」で素通しできるが、
		// バイナリはキーを持たない順次読みなので、途中に挿すと
		// 保存済みの .obeffect が全部ずれる
		//------------------------------------------------------------------
		size_t _soundCount = m_soundParts.size();
		if (a_ar.BeginArray("SoundParts", _soundCount))
		{
			if (a_ar.IsLoading())
			{
				m_soundParts.assign(_soundCount, EffectSoundPart{});
			}

			for (size_t _i = 0; _i < _soundCount; ++_i)
			{
				if (a_ar.BeginObject(_i))
				{
					m_soundParts[_i].Archive(a_ar);
					a_ar.EndObject();
				}
			}
			a_ar.EndArray();

			// 実体側(EffectInstance)の声の席が固定長なので、超えたぶんは鳴らせない
			if (a_ar.IsLoading() && m_soundParts.size() > EFFECT_SOUND_MAX)
			{
				m_soundParts.resize(EFFECT_SOUND_MAX);
			}
		}
	}

	void EffectAsset::Save(const std::string& a_baseFilePath)
	{
		auto _fileDir = Engine::File::GetDirFromPath(a_baseFilePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_baseFilePath);

		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "effect");
		Archive(_ar);
	}

	void EffectAsset::ResolveReferences()
	{
		auto& _resourceManager = ResourceManager::Instance();

		for (auto& _part : m_particleParts)
		{
			if (!_part.IsValid())
			{
				_part.particleHandle = {};
				continue;
			}
			_part.particleHandle = _resourceManager.LoadImmediate<ParticlesAsset>(_part.particleGUID);
		}

		for (auto& _part : m_meshParts)
		{
			if (!_part.IsValid())
			{
				_part.modelHandle = {};
				continue;
			}
			_part.modelHandle = _resourceManager.LoadImmediate<Model>(_part.modelGUID);
		}

		// 波形はここで読んでおく。
		// 実際に鳴らすのは声(SoundInstance)の方で、そちらは使う側が1つずつ持つが、
		// 元の波形は全員で共有できる。持っておかないと
		// 初めて鳴らすエフェクトが湧いた瞬間に読み込みが走る
		for (auto& _part : m_soundParts)
		{
			if (!_part.IsValid())
			{
				_part.soundHandle = {};
				continue;
			}
			_part.soundHandle = _resourceManager.LoadImmediate<Sound>(_part.soundGUID);
		}
	}

	//======================================================================================
	// EffectAsset : 再生
	//======================================================================================
	void EffectAsset::CreateSoundInstances(Engine::Audio::AudioManager& a_audioManager, EffectInstance& a_inst) const
	{
		// 作り直しでも漏らさないよう、まず持っているものを返す。
		// 返した時点で「何から発行したか」も空になるので、
		// 下の Sync は全スロットを食い違いとみなして作り直す
		a_inst.ReleaseSounds(a_audioManager);

		SyncSoundInstances(a_audioManager, a_inst);
	}

	void EffectAsset::SyncSoundInstances(Engine::Audio::AudioManager& a_audioManager, EffectInstance& a_inst) const
	{
		for (size_t _i = 0; _i < EFFECT_SOUND_MAX; ++_i)
		{
			//----------------------------------------------------------
			// このスロットに欲しい声
			//
			// パーツが無い/音が空のスロットは「声なし」が正しい姿。
			// エディターでパーツを消したときに、古い声が残らないようにする
			//----------------------------------------------------------
			Engine::GUID _wantGUID = Engine::DefaultGUID;
			bool _want3D = false;

			const EffectSoundPart* _pPart = nullptr;
			if (_i < m_soundParts.size() && m_soundParts[_i].IsValid())
			{
				_pPart = &m_soundParts[_i];
				_wantGUID = _pPart->soundGUID;
				_want3D = _pPart->is3DSound;
			}

			// 食い違っていなければそのまま使う(ほとんどのフレームはこちら)
			if (a_inst.soundSourceGUID[_i] == _wantGUID &&
				a_inst.soundSource3D[_i] == _want3D)
			{
				continue;
			}

			// 古い声を返してから作り直す。
			// 3D で鳴らすかは発行時にしか決められないので、
			// 入り切りを変えたときも作り直しになる
			// (2Dで作ったインスタンスに Apply3D を掛けると DirectXTK が例外を投げる)
			a_audioManager.ReleaseSoundInstance(a_inst.soundHandles[_i]);
			a_inst.soundHandles[_i] = {};
			a_inst.soundTriggered[_i] = false;

			a_inst.soundSourceGUID[_i] = _wantGUID;
			a_inst.soundSource3D[_i] = _want3D;

			if (!_pPart) continue;

			a_inst.soundHandles[_i] = a_audioManager.RequestSoundInstance(_wantGUID, _want3D);

			if (auto* _pInstance = a_audioManager.RefInstance(a_inst.soundHandles[_i]))
			{
				_pInstance->SetVolume(_pPart->vol);
				_pInstance->SetCurveDistanceScaler(_pPart->distanceScaler);
			}
		}
	}

	void EffectAsset::Play(EffectInstance& a_inst) const
	{
		a_inst.Reset();
		a_inst.isPlaying = true;
	}

	void EffectAsset::Stop(EffectInstance& a_inst, Engine::Audio::AudioManager* a_pAudioManager) const
	{
		a_inst.isPlaying = false;

		// 止めたフレームに出しかけていたぶんは捨てる
		for (size_t _i = 0; _i < EFFECT_PARTICLE_MAX; ++_i)
		{
			a_inst.pendingEmit[_i] = 0;
			a_inst.wasEmitting[_i] = false;
		}

		//------------------------------------------------------------------
		// 鳴っている音
		//
		// 止めるのはループを掛けたものだけ。噴射音のように鳴りっぱなしのものは
		// ここで止めないと吹かすのをやめても鳴り続ける。
		// 一方で単発音は鳴らしきらせる。点火の「ボッ」のような短い音まで切ると、
		// 出し入れの激しい演出で音がぶつ切りになるため
		//------------------------------------------------------------------
		if (a_pAudioManager)
		{
			const size_t _count = std::min<size_t>(m_soundParts.size(), EFFECT_SOUND_MAX);
			for (size_t _i = 0; _i < _count; ++_i)
			{
				if (!m_soundParts[_i].IsValid()) continue;

				if (m_soundParts[_i].isLoop)
				{
					if (auto* _pInstance = a_pAudioManager->RefInstance(a_inst.soundHandles[_i]))
					{
						_pInstance->Stop();
					}
				}

				// 次に再生したときは鳴らし直す
				a_inst.soundTriggered[_i] = false;
			}
		}
	}

	void EffectAsset::Update(EffectInstance& a_inst, float a_dt, Engine::Audio::AudioManager* a_pAudioManager) const
	{
		// 既定は今フレーム発生なし
		for (size_t _i = 0; _i < EFFECT_PARTICLE_MAX; ++_i)
		{
			a_inst.pendingEmit[_i] = 0;
		}

		if (!a_inst.isPlaying) return;

		a_inst.elapsed += a_dt;

		const size_t _count = std::min<size_t>(m_particleParts.size(), EFFECT_PARTICLE_MAX);
		for (size_t _i = 0; _i < _count; ++_i)
		{
			const EffectParticlePart& _part = m_particleParts[_i];

			// 中身が入っていないパーツは飛ばす
			if (!_part.IsValid()) continue;

			// 出す時間帯に入っていない(待ち時間中・終了済み)
			if (!_part.timing.IsActiveAt(a_inst.elapsed))
			{
				a_inst.rateAccum[_i] = 0.0f;
				a_inst.wasEmitting[_i] = false;
				continue;
			}

			if (_part.emitRate > 0.0f)
			{
				// ---- 連続発生 : 毎秒 emitRate 回 ----
				a_inst.rateAccum[_i] += a_dt;
				const float _interval = 1.0f / _part.emitRate;

				int _bursts = 0;
				// 溜まった分だけ発生させ、端数は残す。暴走防止に上限を設ける
				while (a_inst.rateAccum[_i] >= _interval && _bursts < 64)
				{
					a_inst.rateAccum[_i] -= _interval;
					++_bursts;
				}
				a_inst.pendingEmit[_i] = _bursts * _part.emitCount;
			}
			else
			{
				// ---- バースト : 出し始めのフレームで一度だけ ----
				if (!a_inst.wasEmitting[_i])
				{
					a_inst.pendingEmit[_i] = _part.emitCount;
				}
			}

			a_inst.wasEmitting[_i] = true;
		}

		//------------------------------------------------------------------
		// サウンド
		//
		// パーティクルと同じ時間軸(elapsed)で、StartDelay が来たものから鳴らす。
		// 「絵と音を1枚のアセットにまとめる」のが狙いなので、
		// 鳴らす側は再生を伝えるだけでよく、音を別に鳴らしに行かなくてよい。
		//
		// 単発音は1回鳴らすだけ。毎フレーム Play を呼ぶと頭出しが繰り返されて
		// 音が伸びないので、鳴らした印(soundTriggered)で1回に抑える
		//------------------------------------------------------------------
		if (a_pAudioManager)
		{
			// エディターで音を差し替えたぶんをここで拾う。
			// 食い違っているスロットだけ作り直すので、毎フレーム通してよい
			SyncSoundInstances(*a_pAudioManager, a_inst);

			const size_t _soundCount = std::min<size_t>(m_soundParts.size(), EFFECT_SOUND_MAX);
			for (size_t _i = 0; _i < _soundCount; ++_i)
			{
				const EffectSoundPart& _part = m_soundParts[_i];
				if (!_part.IsValid()) continue;

				auto* _pInstance = a_pAudioManager->RefInstance(a_inst.soundHandles[_i]);
				if (!_pInstance) continue;

				// まだ鳴らす時間になっていない
				if (a_inst.elapsed < _part.timing.startDelay) continue;

				// ループ音は Duration で止める(0 なら止めるまで鳴りっぱなし)。
				// 単発音は自分で鳴り終わるので、長さの指定は見ない
				if (_part.isLoop && _part.timing.IsFinishedAt(a_inst.elapsed))
				{
					if (a_inst.soundTriggered[_i])
					{
						_pInstance->Stop();
						a_inst.soundTriggered[_i] = false;
					}
					continue;
				}

				if (a_inst.soundTriggered[_i]) continue;
				a_inst.soundTriggered[_i] = true;

				// 3D 指定でも、発行が2Dだったなら2Dで鳴らす
				// (3D かどうかは発行時にしか決められないため。CreateSoundInstances 参照)
				if (_part.is3DSound && _pInstance->Is3D())
				{
					_pInstance->SetCurveDistanceScaler(_part.distanceScaler);
					_pInstance->Play3D(a_inst.soundPos, _part.isLoop);
				}
				else
				{
					_pInstance->Play(_part.isLoop);
				}

				// Play3D は音量を 1 に戻すので、鳴らした後に入れ直す。
				// エディターで音量をいじったぶんもここで乗る
				_pInstance->SetVolume(_part.vol);
			}
		}
	}

	bool EffectAsset::IsFinished(const EffectInstance& a_inst, Engine::Audio::AudioManager* a_pAudioManager) const
	{
		// まだ一度も再生していない/止めたものは「出し切った」ではない。
		// ここで true を返すと、再生前のエフェクトが
		// destroyOnFinish で湧いた瞬間に消える
		if (!a_inst.isPlaying) return false;

		for (const auto& _part : m_particleParts)
		{
			if (!_part.IsValid()) continue;
			if (!_part.timing.IsFinishedAt(a_inst.elapsed)) return false;
		}
		for (const auto& _part : m_meshParts)
		{
			if (!_part.IsValid()) continue;
			if (!_part.timing.IsFinishedAt(a_inst.elapsed)) return false;
		}

		//------------------------------------------------------------------
		// サウンド
		//
		// 絵より音の方が長いことは珍しくない(爆発の余韻など)。
		// 音を見ないと、絵が消えたフレームで destroyOnFinish がエンティティごと
		// 消してしまい、借りている声も返却されて音がぶつ切りになる。
		//
		// 待つかどうかはパーツごとの isWaitFinish で選べる。
		// (BGM的に長い音を足したときに、エフェクトがいつまでも消えなくなるため)
		//------------------------------------------------------------------
		if (a_pAudioManager)
		{
			const size_t _soundCount = std::min<size_t>(m_soundParts.size(), EFFECT_SOUND_MAX);
			for (size_t _i = 0; _i < _soundCount; ++_i)
			{
				const EffectSoundPart& _part = m_soundParts[_i];
				if (!_part.IsValid()) continue;
				if (!_part.isWaitFinish) continue;

				// ループ音は止めるまで終わらない。パーティクルの出しっぱなしと同じ扱い
				if (_part.isLoop && _part.timing.duration <= 0.0f) return false;

				// 鳴らす前(待ち時間中)は、まだ終わっていない
				if (!a_inst.soundTriggered[_i])
				{
					if (a_inst.elapsed < _part.timing.startDelay) return false;
					continue;
				}

				auto* _pInstance = a_pAudioManager->RefInstance(a_inst.soundHandles[_i]);
				if (!_pInstance) continue;

				if (_pInstance->IsPlay()) return false;
			}
		}

		return true;
	}

	bool EffectAsset::BuildMeshDraw(
		size_t a_index,
		const EffectInstance& a_inst,
		const Math::Matrix& a_ownerWorld,
		Math::Matrix& a_outWorld,
		Math::Color& a_outColorScale,
		Math::Vector3& a_outEmissiveAdd
	) const
	{
		if (!a_inst.isPlaying) return false;
		if (a_index >= m_meshParts.size()) return false;

		const EffectMeshPart& _part = m_meshParts[a_index];
		if (!_part.IsValid()) return false;
		if (!_part.timing.IsActiveAt(a_inst.elapsed)) return false;

		// 出している区間の進み具合(0〜1)。出しっぱなしなら常に 0 = 開始時の見た目のまま
		const float _t = _part.timing.GetProgressAt(a_inst.elapsed);

		// ---- スケール : 開始値から終値へ寄せる ----
		const Math::Vector3 _scale =
		{
			std::lerp(_part.scale.x, _part.scale.x * _part.endScale.x, _t),
			std::lerp(_part.scale.y, _part.scale.y * _part.endScale.y, _t),
			std::lerp(_part.scale.z, _part.scale.z * _part.endScale.z, _t),
		};

		// ---- 配置 : 相手の行列基準のローカル配置を合成する ----
		const DirectX::XMMATRIX _local =
			DirectX::XMMatrixScaling(_scale.x, _scale.y, _scale.z) *
			DirectX::XMMatrixRotationRollPitchYaw(
				DirectX::XMConvertToRadians(_part.rotation.x),
				DirectX::XMConvertToRadians(_part.rotation.y),
				DirectX::XMConvertToRadians(_part.rotation.z)) *
			DirectX::XMMatrixTranslation(_part.posOffset.x, _part.posOffset.y, _part.posOffset.z);

		a_outWorld = Math::DX::StoreMatrix(_local * Math::DX::Load(a_ownerWorld));

		// ---- 色 : アルファだけ終値へ寄せる ----
		a_outColorScale = _part.colorScale;
		a_outColorScale.a = std::lerp(_part.colorScale.a, _part.endAlpha, _t);

		// ---- 発光 : 強さを終値へ寄せて色に掛ける ----
		const float _intensity = std::lerp(_part.emissiveIntensity, _part.endEmissiveIntensity, _t);
		a_outEmissiveAdd =
		{
			_part.emissiveColor.x * _intensity,
			_part.emissiveColor.y * _intensity,
			_part.emissiveColor.z * _intensity,
		};

		return true;
	}
}
