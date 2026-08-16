#include "SceneSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/ECS/World/World.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

#include "Application/Components/Hierarchy/SpownerComponent.h"
#include "Application/Utility/PrefabSpawnHelper.h"

//==========================================================================================
// SceneSequence
//
// ウェーブの進行:
//   1) 生存数を数える      … 出したエンティティに付けた SpownerComponent を毎フレーム集計
//   2) 全滅判定を進める    … 生存数が 0 になった瞬間の時刻を覚えておく
//   3) 条件を満たしたウェーブを出す
//
// 「出した直後は生存数 0」問題に注意。プレハブの実体化は遅延生成(次の BeginFrame)なので、
// 出した次の瞬間に数えると 0 になり、全滅扱いで次のウェーブが即座に走ってしまう。
// そのため一度でも生存を確認する(isConfirmed)まで全滅とみなさない。
// 実体化に失敗した(プレハブが空など)場合に詰まらないよう、確認には時間切れを設けてある。
//==========================================================================================
namespace App::Object
{
	namespace
	{
		// 実体化を待つ時間(秒)。これを過ぎても生存が確認できなければ全滅扱いにして次へ進む
		constexpr float SPOWAN_CONFIRM_TIMEOUT = 2.0f;

		// 出現位置マーカーの大きさ(m)
		constexpr float MARKER_SIZE = 1.0f;

		// dir(水平方向)から左手系 +Z 前方のクォータニオンを作る。
		// 長さが無い/真上を向いている場合は回さない(プレハブの向きのまま)。
		DXSM::Quaternion MakeYawQuat(const DXSM::Vector3& a_dir, bool& a_outHasRotation)
		{
			DXSM::Vector3 _flat = { a_dir.x, 0.0f, a_dir.z };
			if (_flat.LengthSquared() <= 1e-6f)
			{
				a_outHasRotation = false;
				return DXSM::Quaternion::Identity;
			}

			a_outHasRotation = true;

			// 左手系 +Z 前方なので Yaw = atan2(x, z)
			const float _yaw = std::atan2(_flat.x, _flat.z);
			return DXSM::Quaternion::CreateFromYawPitchRoll(_yaw, 0.0f, 0.0f);
		}
	}

	void SceneSequence::Update(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pWorld) return;

		m_time += a_context.dt;

		// 生存数 → 全滅判定 → 出現、の順に進める
		UpdateAliveCount(a_context);
		UpdateClearState();

		for (size_t _i = 0; _i < m_weaves.size(); ++_i)
		{
			if (m_weaves[_i].isSpowan) continue;
			if (!CanSpowan(_i)) continue;

			Spowan(a_context, _i);
		}

		DrawSpowanMarker(a_context);
	}

	//======================================================================================
	// 生存数の集計
	//--------------------------------------------------------------------------------------
	// 自分のGUIDが付いた生存エンティティを、ウェーブ番号ごとに数える。
	// ActiveTag も条件に入れているので、解放待ち(ReleaseTag)の個体は数えない。
	//======================================================================================
	void SceneSequence::UpdateAliveCount(Engine::GameObject::ObjectContext& a_context)
	{
		for (Weave& _weave : m_weaves) _weave.aliveCount = 0;

		const Engine::GUID _selfGUID = GetGUID();
		if (!_selfGUID.IsValid()) return;

		a_context.pWorld->ForEach<const ActiveTag, const SpownerComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const SpownerComponent* a_spownerArray
			)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					const SpownerComponent& _spowner = a_spownerArray[_i];

					if (!(_spowner.spownerGUID == _selfGUID)) continue;
					if (_spowner.waveIndex < 0) continue;
					if (static_cast<size_t>(_spowner.waveIndex) >= m_weaves.size()) continue;

					++m_weaves[_spowner.waveIndex].aliveCount;
				}
			}
		);
	}

	//======================================================================================
	// 全滅判定
	//======================================================================================
	void SceneSequence::UpdateClearState()
	{
		for (Weave& _weave : m_weaves)
		{
			if (!_weave.isSpowan || _weave.isCleared) continue;

			// 1体も出せなかったウェーブは待つ相手がいないので即クリア扱い
			if (_weave.spowanCount <= 0)
			{
				_weave.isCleared   = true;
				_weave.clearedTime = m_time;
				continue;
			}

			// 実体化を確認するまでは全滅とみなさない(遅延生成の1フレームずれ対策)
			if (_weave.aliveCount > 0)
			{
				_weave.isConfirmed = true;
				continue;
			}

			const bool _isTimeout = (m_time - _weave.spowanTime) >= SPOWAN_CONFIRM_TIMEOUT;
			if (!_weave.isConfirmed && !_isTimeout) continue;

			_weave.isCleared   = true;
			_weave.clearedTime = m_time;
		}
	}

	//======================================================================================
	// 出現条件
	//--------------------------------------------------------------------------------------
	//   isAnnihilation = false : シーン開始から timing 秒
	//   isAnnihilation = true  : 前のウェーブが全滅してから timing 秒
	//                            (先頭のウェーブは待つ相手がいないので時間だけ見る)
	//======================================================================================
	bool SceneSequence::CanSpowan(size_t a_index) const
	{
		if (a_index >= m_weaves.size()) return false;

		const Weave& _weave = m_weaves[a_index];

		if (!_weave.isAnnihilation || a_index == 0)
		{
			return m_time >= _weave.timing;
		}

		const Weave& _prev = m_weaves[a_index - 1];
		if (!_prev.isCleared) return false;

		return (m_time - _prev.clearedTime) >= _weave.timing;
	}

	//======================================================================================
	// 出現
	//======================================================================================
	void SceneSequence::Spowan(Engine::GameObject::ObjectContext& a_context, size_t a_index)
	{
		if (a_index >= m_weaves.size()) return;
		if (!a_context.pWorld) return;
		if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

		Weave& _weave = m_weaves[a_index];

		// 出現処理はこのフレームで完了させる。プレハブが引けずに0体でも進める
		// (待ち続けるとウェーブの連鎖が止まってしまうため)
		_weave.isSpowan    = true;
		_weave.spowanTime  = m_time;
		_weave.spowanCount = 0;

		for (SpowanSettings& _settings : _weave.spowanEntities)
		{
			App::Utility::SpawnParams _params = {};
			_params.pos = _settings.pos;

			bool _hasRotation = false;
			const DXSM::Quaternion _quat = MakeYawQuat(_settings.dir, _hasRotation);
			_params.quat               = _quat;
			_params.isOverrideRotation = _hasRotation;

			// 全滅判定用の印。ウェーブ番号まで入れて自分の中で区別する
			_params.spownerGUID = GetGUID();
			_params.waveIndex   = static_cast<int>(a_index);

			const bool _isSpowaned = App::Utility::SpawnPrefab(
				*a_context.pWorld,
				*a_context.pServices->pResourceManager,
				_settings.spowanEntityGUID,
				_settings.spowanPrefabHandle,
				_params);

			if (_isSpowaned) ++_weave.spowanCount;
		}
	}

	//======================================================================================
	// 進行状況のリセット(エディターから何度も試せるように)
	//======================================================================================
	void SceneSequence::ResetProgress()
	{
		m_time = 0.0f;

		for (Weave& _weave : m_weaves)
		{
			_weave.isSpowan    = false;
			_weave.spowanCount = 0;
			_weave.aliveCount  = 0;
			_weave.isConfirmed = false;
			_weave.isCleared   = false;
			_weave.spowanTime  = 0.0f;
			_weave.clearedTime = 0.0f;
		}
	}

	//======================================================================================
	// デバッグ描画 : まだ出していないウェーブの出現位置と向きを出す
	//======================================================================================
	void SceneSequence::DrawSpowanMarker(Engine::GameObject::ObjectContext& a_context) const
	{
		if (!a_context.pServices) return;

		auto* _pEditor = a_context.pServices->pMainEditor;
		if (!_pEditor) return;

		for (const Weave& _weave : m_weaves)
		{
			if (_weave.isSpowan) continue;

			for (const SpowanSettings& _settings : _weave.spowanEntities)
			{
				const DXSM::Vector3 _pos = _settings.pos;

				// 位置の十字
				_pEditor->DrawLine(
					_pos - DXSM::Vector3(MARKER_SIZE, 0.0f, 0.0f),
					_pos + DXSM::Vector3(MARKER_SIZE, 0.0f, 0.0f), Engine::Color::GREEN);
				_pEditor->DrawLine(
					_pos - DXSM::Vector3(0.0f, 0.0f, MARKER_SIZE),
					_pos + DXSM::Vector3(0.0f, 0.0f, MARKER_SIZE), Engine::Color::GREEN);
				_pEditor->DrawLine(
					_pos, _pos + DXSM::Vector3(0.0f, MARKER_SIZE, 0.0f), Engine::Color::GREEN);

				// 向き
				DXSM::Vector3 _dir = { _settings.dir.x, 0.0f, _settings.dir.z };
				if (_dir.LengthSquared() > 1e-6f)
				{
					_dir.Normalize();
					_pEditor->DrawLine(_pos, _pos + _dir * (MARKER_SIZE * 2.0f), Engine::Color::BLUE);
				}
			}
		}
	}

	//======================================================================================
	// シリアライズ : 設定値だけを保存する(進行状況は保存しない)
	//======================================================================================
	void SceneSequence::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		size_t _weaveCount = m_weaves.size();
		if (!a_ar.BeginArray("Weaves", _weaveCount)) return;

		m_weaves.resize(_weaveCount);

		for (size_t _i = 0; _i < _weaveCount; ++_i)
		{
			if (!a_ar.BeginObject(_i)) continue;

			Weave& _weave = m_weaves[_i];
			a_ar.Field("timing", _weave.timing);
			a_ar.Field("isAnnihilation", _weave.isAnnihilation);

			size_t _spowanCount = _weave.spowanEntities.size();
			if (a_ar.BeginArray("Spowans", _spowanCount))
			{
				_weave.spowanEntities.resize(_spowanCount);

				for (size_t _s = 0; _s < _spowanCount; ++_s)
				{
					if (!a_ar.BeginObject(_s)) continue;

					SpowanSettings& _settings = _weave.spowanEntities[_s];
					a_ar.GUIDField("prefabGUID", _settings.spowanEntityGUID);
					a_ar.Field("pos", _settings.pos);
					a_ar.Field("dir", _settings.dir);

					a_ar.EndObject();
				}
				a_ar.EndArray();
			}

			a_ar.EndObject();
		}
		a_ar.EndArray();
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void SceneSequence::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::Text("Time : %.2f", m_time);
		ImGui::SameLine();
		if (ImGui::Button("Reset Progress")) ResetProgress();

		ImGui::Separator();

		if (ImGui::Button("Add Wave")) m_weaves.emplace_back();

		int _removeWaveIndex = -1;

		for (size_t _i = 0; _i < m_weaves.size(); ++_i)
		{
			Weave& _weave = m_weaves[_i];

			ImGui::PushID(static_cast<int>(_i));

			const std::string _label = "Wave " + std::to_string(_i);
			if (ImGui::CollapsingHeader(_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				// ---- 出現条件 ----
				ImGui::Checkbox("IsAnnihilation", &_weave.isAnnihilation);
				ImGui::SameLine();
				ImGui::TextDisabled(_weave.isAnnihilation ? "(after prev cleared)" : "(from scene start)");

				ImGui::DragFloat("Timing", &_weave.timing, 0.1f, 0.0f, 3600.0f);

				// ---- 進行状況 ----
				ImGui::TextDisabled("Spowaned : %s / Alive : %d / Cleared : %s",
					_weave.isSpowan ? "yes" : "no",
					_weave.aliveCount,
					_weave.isCleared ? "yes" : "no");

				// ---- 出現させるエンティティ ----
				if (ImGui::Button("Add Spowan")) _weave.spowanEntities.emplace_back();

				int _removeSpowanIndex = -1;

				for (size_t _s = 0; _s < _weave.spowanEntities.size(); ++_s)
				{
					SpowanSettings& _settings = _weave.spowanEntities[_s];

					ImGui::PushID(static_cast<int>(_s));
					ImGui::Separator();

					// プレハブを選び直したらハンドルを捨てて解決し直させる
					if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
						"Prefab", "Prefab", _settings.spowanEntityGUID))
					{
						_settings.spowanPrefabHandle = {};
					}

					ImGui::DragFloat3("Pos", &_settings.pos.x, 0.1f);
					ImGui::DragFloat3("Dir", &_settings.dir.x, 0.01f);

					if (ImGui::Button("Remove Spowan")) _removeSpowanIndex = static_cast<int>(_s);

					ImGui::PopID();
				}

				if (_removeSpowanIndex >= 0)
				{
					_weave.spowanEntities.erase(
						_weave.spowanEntities.begin() + _removeSpowanIndex);
				}

				ImGui::Separator();
				if (ImGui::Button("Remove Wave")) _removeWaveIndex = static_cast<int>(_i);
			}

			ImGui::PopID();
		}

		if (_removeWaveIndex >= 0)
		{
			m_weaves.erase(m_weaves.begin() + _removeWaveIndex);

			// ウェーブ番号がずれるので、出し済みの個体との対応も作り直す
			ResetProgress();
		}
	}
}
