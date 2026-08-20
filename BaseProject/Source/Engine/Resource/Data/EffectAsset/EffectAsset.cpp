#include "EffectAsset.h"

#include "../../Manager/ResourceManager/ResourceManager.h"

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
	}

	//======================================================================================
	// EffectAsset : 再生
	//======================================================================================
	void EffectAsset::Play(EffectInstance& a_inst) const
	{
		a_inst.Reset();
		a_inst.isPlaying = true;
	}

	void EffectAsset::Stop(EffectInstance& a_inst) const
	{
		a_inst.isPlaying = false;

		// 止めたフレームに出しかけていたぶんは捨てる
		for (size_t _i = 0; _i < EFFECT_PARTICLE_MAX; ++_i)
		{
			a_inst.pendingEmit[_i] = 0;
			a_inst.wasEmitting[_i] = false;
		}
	}

	void EffectAsset::Update(EffectInstance& a_inst, float a_dt) const
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
	}

	bool EffectAsset::IsFinished(const EffectInstance& a_inst) const
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
