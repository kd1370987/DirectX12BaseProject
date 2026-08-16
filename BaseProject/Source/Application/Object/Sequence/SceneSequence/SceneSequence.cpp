#include "SceneSequence.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/ECS/World/World.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

#include "Application/Components/Hierarchy/SpawnerComponent.h"
#include "Application/Utility/PrefabSpawnHelper.h"

//==========================================================================================
// SceneSequence
//
// ウェーブの進行:
//   1) 生存数を数える      … 出したエンティティに付けた SpawnerComponent を毎フレーム集計
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
		constexpr float SPAWN_CONFIRM_TIMEOUT = 2.0f;

		// 出現位置マーカーの大きさ(m)
		constexpr float MARKER_SIZE = 1.0f;

		// 出し終えたウェーブのマーカー色(これから出るものと見分けるため落とした色)
		constexpr DirectX::XMFLOAT4 DIMMED_COLOR = { 0.35f, 0.35f, 0.35f, 1.0f };

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

		for (size_t _i = 0; _i < m_waves.size(); ++_i)
		{
			if (m_waves[_i].isSpawned) continue;
			if (!CanSpawn(_i)) continue;

			Spawn(a_context, _i);
		}

		DrawSpawnMarker(a_context);
	}

	//======================================================================================
	// 生存数の集計
	//--------------------------------------------------------------------------------------
	// 自分のGUIDが付いた生存エンティティを、ウェーブ番号ごとに数える。
	// ActiveTag も条件に入れているので、解放待ち(ReleaseTag)の個体は数えない。
	//======================================================================================
	void SceneSequence::UpdateAliveCount(Engine::GameObject::ObjectContext& a_context)
	{
		for (Wave& _wave : m_waves) _wave.aliveCount = 0;

		const Engine::GUID _selfGUID = GetGUID();
		if (!_selfGUID.IsValid()) return;

		a_context.pWorld->ForEach<const ActiveTag, const SpawnerComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const SpawnerComponent* a_spawnerArray
			)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					const SpawnerComponent& _spawner = a_spawnerArray[_i];

					if (!(_spawner.spawnerGUID == _selfGUID)) continue;
					if (_spawner.waveIndex < 0) continue;
					if (static_cast<size_t>(_spawner.waveIndex) >= m_waves.size()) continue;

					++m_waves[_spawner.waveIndex].aliveCount;
				}
			}
		);
	}

	//======================================================================================
	// 全滅判定
	//======================================================================================
	void SceneSequence::UpdateClearState()
	{
		for (Wave& _wave : m_waves)
		{
			if (!_wave.isSpawned || _wave.isCleared) continue;

			// 1体も出せなかったウェーブは待つ相手がいないので即クリア扱い
			if (_wave.spawnCount <= 0)
			{
				_wave.isCleared   = true;
				_wave.clearedTime = m_time;
				continue;
			}

			// 実体化を確認するまでは全滅とみなさない(遅延生成の1フレームずれ対策)
			if (_wave.aliveCount > 0)
			{
				_wave.isConfirmed = true;
				continue;
			}

			const bool _isTimeout = (m_time - _wave.spawnTime) >= SPAWN_CONFIRM_TIMEOUT;
			if (!_wave.isConfirmed && !_isTimeout) continue;

			_wave.isCleared   = true;
			_wave.clearedTime = m_time;
		}
	}

	//======================================================================================
	// 出現条件
	//--------------------------------------------------------------------------------------
	//   isAnnihilation = false : シーン開始から timing 秒
	//   isAnnihilation = true  : 前のウェーブが全滅してから timing 秒
	//                            (先頭のウェーブは待つ相手がいないので時間だけ見る)
	//======================================================================================
	bool SceneSequence::CanSpawn(size_t a_index) const
	{
		if (a_index >= m_waves.size()) return false;

		const Wave& _wave = m_waves[a_index];

		if (!_wave.isAnnihilation || a_index == 0)
		{
			return m_time >= _wave.timing;
		}

		const Wave& _prev = m_waves[a_index - 1];
		if (!_prev.isCleared) return false;

		return (m_time - _prev.clearedTime) >= _wave.timing;
	}

	//======================================================================================
	// 出現
	//======================================================================================
	void SceneSequence::Spawn(Engine::GameObject::ObjectContext& a_context, size_t a_index)
	{
		if (a_index >= m_waves.size()) return;
		if (!a_context.pWorld) return;
		if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

		Wave& _wave = m_waves[a_index];

		// 出現処理はこのフレームで完了させる。プレハブが引けずに0体でも進める
		// (待ち続けるとウェーブの連鎖が止まってしまうため)
		_wave.isSpawned  = true;
		_wave.spawnTime  = m_time;
		_wave.spawnCount = 0;

		for (SpawnSettings& _settings : _wave.spawnEntities)
		{
			App::Utility::SpawnParams _params = {};

			// 出現位置はウェーブ位置からの相対。ウェーブごと動かせるようにするため
			_params.pos = _wave.pos + _settings.pos;

			bool _hasRotation = false;
			const DXSM::Quaternion _quat = MakeYawQuat(_settings.dir, _hasRotation);
			_params.quat               = _quat;
			_params.isOverrideRotation = _hasRotation;

			// 全滅判定用の印。ウェーブ番号まで入れて自分の中で区別する
			_params.spawnerGUID = GetGUID();
			_params.waveIndex   = static_cast<int>(a_index);

			const bool _isSpawned = App::Utility::SpawnPrefab(
				*a_context.pWorld,
				*a_context.pServices->pResourceManager,
				_settings.spawnEntityGUID,
				_settings.spawnPrefabHandle,
				_params);

			if (_isSpawned) ++_wave.spawnCount;
		}
	}

	//======================================================================================
	// 進行状況のリセット(エディターから何度も試せるように)
	//======================================================================================
	void SceneSequence::ResetProgress()
	{
		m_time = 0.0f;

		for (Wave& _wave : m_waves)
		{
			_wave.isSpawned   = false;
			_wave.spawnCount  = 0;
			_wave.aliveCount  = 0;
			_wave.isConfirmed = false;
			_wave.isCleared   = false;
			_wave.spawnTime   = 0.0f;
			_wave.clearedTime = 0.0f;
		}
	}

	//======================================================================================
	// ギズモの対象を外す(添え字がずれる操作のあとに呼ぶ)
	//======================================================================================
	void SceneSequence::ClearGizmoTarget()
	{
		m_gizmoWaveIndex  = -1;
		m_gizmoSpawnIndex = -1;
	}

	//======================================================================================
	// デバッグ描画 : ウェーブの基準位置と、そこからの出現位置・向きを出す
	//--------------------------------------------------------------------------------------
	// 出し終えたウェーブは灰色にして、これから出るものと見分けられるようにする。
	// ギズモで選んでいる点は白。
	//======================================================================================
	void SceneSequence::DrawSpawnMarker(Engine::GameObject::ObjectContext& a_context) const
	{
		if (!a_context.pServices) return;

		auto* _pEditor = a_context.pServices->pMainEditor;
		if (!_pEditor) return;

		// 位置の十字(縦は上方向だけ伸ばして地面基準に見せる)
		auto _drawCross = [&](const DXSM::Vector3& a_pos, float a_size, const DXSM::Color& a_color)
			{
				_pEditor->DrawLine(
					a_pos - DXSM::Vector3(a_size, 0.0f, 0.0f),
					a_pos + DXSM::Vector3(a_size, 0.0f, 0.0f), a_color);
				_pEditor->DrawLine(
					a_pos - DXSM::Vector3(0.0f, 0.0f, a_size),
					a_pos + DXSM::Vector3(0.0f, 0.0f, a_size), a_color);
				_pEditor->DrawLine(
					a_pos, a_pos + DXSM::Vector3(0.0f, a_size, 0.0f), a_color);
			};

		for (size_t _i = 0; _i < m_waves.size(); ++_i)
		{
			const Wave& _wave = m_waves[_i];

			const bool _isGizmoWave = (m_gizmoWaveIndex == static_cast<int>(_i));
			const DXSM::Color _baseColor = _wave.isSpawned ? DIMMED_COLOR : Engine::Color::GREEN;

			// ウェーブの基準位置は少し大きめに
			_drawCross(_wave.pos, MARKER_SIZE * 2.0f,
				(_isGizmoWave && m_gizmoSpawnIndex < 0) ? Engine::Color::WHITE : _baseColor);

			for (size_t _s = 0; _s < _wave.spawnEntities.size(); ++_s)
			{
				const SpawnSettings& _settings = _wave.spawnEntities[_s];

				// 出現位置はウェーブからの相対
				const DXSM::Vector3 _pos = _wave.pos + _settings.pos;

				const bool _isGizmoSpawn =
					_isGizmoWave && (m_gizmoSpawnIndex == static_cast<int>(_s));
				const DXSM::Color _color = _isGizmoSpawn ? Engine::Color::WHITE : _baseColor;

				_drawCross(_pos, MARKER_SIZE, _color);

				// ウェーブとのつながり
				_pEditor->DrawLine(_wave.pos, _pos, _baseColor);

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
	// ギズモ : インスペクタで選んだ1点を動かす
	//--------------------------------------------------------------------------------------
	// ImGuizmo は一度に1つの行列しか操作できないので、対象はインスペクタ側で選ぶ。
	// SetDrawlist / SetRect は呼び出し元(SceneViewPanel)が済ませてある。
	//======================================================================================
	bool SceneSequence::DrawGizmo(
		const Engine::GameObject::ObjectGizmoContext& a_ctx,
		Engine::GameObject::ObjectContext& a_context)
	{
		if (m_gizmoWaveIndex < 0) return false;
		if (static_cast<size_t>(m_gizmoWaveIndex) >= m_waves.size()) return false;

		Wave& _wave = m_waves[m_gizmoWaveIndex];

		// 動かす座標と、その座標が乗っている基準(ワールド)
		DXSM::Vector3* _pTargetPos = &_wave.pos;
		DXSM::Vector3  _origin     = {};

		if (m_gizmoSpawnIndex >= 0)
		{
			if (static_cast<size_t>(m_gizmoSpawnIndex) >= _wave.spawnEntities.size()) return false;

			_pTargetPos = &_wave.spawnEntities[m_gizmoSpawnIndex].pos;
			_origin     = _wave.pos;	// 出現位置はウェーブからの相対
		}

		const DXSM::Vector3 _worldPos = _origin + *_pTargetPos;

		DirectX::XMFLOAT4X4 _mat = {};
		DirectX::XMStoreFloat4x4(&_mat,
			DirectX::XMMatrixTranslation(_worldPos.x, _worldPos.y, _worldPos.z));

		// Ctrl を押している間だけスナップ(エンティティ用ギズモと同じ操作感)
		float _snapValues[3] = { 1.0f, 1.0f, 1.0f };
		const bool _isSnap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

		ImGuizmo::Manipulate(
			&a_ctx.viewMat._11,
			&a_ctx.projMat._11,
			ImGuizmo::OPERATION::TRANSLATE,
			ImGuizmo::MODE::WORLD,
			&_mat._11,
			nullptr,
			_isSnap ? &_snapValues[0] : nullptr
		);

		if (ImGuizmo::IsUsing())
		{
			// 保持しているのは相対座標なので、基準を引いてから書き戻す
			*_pTargetPos = DXSM::Vector3(_mat._41, _mat._42, _mat._43) - _origin;
		}

		return true;
	}

	//======================================================================================
	// シリアライズ : 設定値だけを保存する(進行状況は保存しない)
	//======================================================================================
	void SceneSequence::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		size_t _waveCount = m_waves.size();
		if (!a_ar.BeginArray("Waves", _waveCount)) return;

		m_waves.resize(_waveCount);

		for (size_t _i = 0; _i < _waveCount; ++_i)
		{
			if (!a_ar.BeginObject(_i)) continue;

			Wave& _wave = m_waves[_i];
			a_ar.Field("pos", _wave.pos);
			a_ar.Field("timing", _wave.timing);
			a_ar.Field("isAnnihilation", _wave.isAnnihilation);

			size_t _spawnCount = _wave.spawnEntities.size();
			if (a_ar.BeginArray("Spawns", _spawnCount))
			{
				_wave.spawnEntities.resize(_spawnCount);

				for (size_t _s = 0; _s < _spawnCount; ++_s)
				{
					if (!a_ar.BeginObject(_s)) continue;

					SpawnSettings& _settings = _wave.spawnEntities[_s];
					a_ar.GUIDField("prefabGUID", _settings.spawnEntityGUID);
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

		ImGui::TextDisabled("Gizmo : select a position below (Ctrl to snap)");

		ImGui::Separator();

		if (ImGui::Button("Add Wave")) m_waves.emplace_back();

		int _removeWaveIndex = -1;

		for (size_t _i = 0; _i < m_waves.size(); ++_i)
		{
			Wave& _wave = m_waves[_i];

			ImGui::PushID(static_cast<int>(_i));

			const std::string _label = "Wave " + std::to_string(_i);
			if (ImGui::CollapsingHeader(_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				// ---- 基準位置 ----
				// ここを動かすと配下の出現位置(相対座標)がまとめて動く
				ImGui::DragFloat3("Wave Pos", &_wave.pos.x, 0.1f);

				ImGui::SameLine();
				const bool _isGizmoWave =
					(m_gizmoWaveIndex == static_cast<int>(_i)) && (m_gizmoSpawnIndex < 0);
				if (ImGui::RadioButton("Gizmo##wave", _isGizmoWave))
				{
					m_gizmoWaveIndex  = static_cast<int>(_i);
					m_gizmoSpawnIndex = -1;
				}

				// ---- 出現条件 ----
				ImGui::Checkbox("IsAnnihilation", &_wave.isAnnihilation);
				ImGui::SameLine();
				ImGui::TextDisabled(_wave.isAnnihilation ? "(after prev cleared)" : "(from scene start)");

				ImGui::DragFloat("Timing", &_wave.timing, 0.1f, 0.0f, 3600.0f);

				// ---- 進行状況 ----
				ImGui::TextDisabled("Spawned : %s / Alive : %d / Cleared : %s",
					_wave.isSpawned ? "yes" : "no",
					_wave.aliveCount,
					_wave.isCleared ? "yes" : "no");

				// ---- 出現させるエンティティ ----
				if (ImGui::Button("Add Spawn")) _wave.spawnEntities.emplace_back();

				int _removeSpawnIndex = -1;

				for (size_t _s = 0; _s < _wave.spawnEntities.size(); ++_s)
				{
					SpawnSettings& _settings = _wave.spawnEntities[_s];

					ImGui::PushID(static_cast<int>(_s));
					ImGui::Separator();

					// プレハブを選び直したらハンドルを捨てて解決し直させる
					if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
						"Prefab", "Prefab", _settings.spawnEntityGUID))
					{
						_settings.spawnPrefabHandle = {};
					}

					// 位置はウェーブからの相対
					ImGui::DragFloat3("Pos (relative)", &_settings.pos.x, 0.1f);

					ImGui::SameLine();
					const bool _isGizmoSpawn =
						(m_gizmoWaveIndex == static_cast<int>(_i)) &&
						(m_gizmoSpawnIndex == static_cast<int>(_s));
					if (ImGui::RadioButton("Gizmo##spawn", _isGizmoSpawn))
					{
						m_gizmoWaveIndex  = static_cast<int>(_i);
						m_gizmoSpawnIndex = static_cast<int>(_s);
					}

					ImGui::DragFloat3("Dir", &_settings.dir.x, 0.01f);
					ImGui::TextDisabled("World : %.2f, %.2f, %.2f",
						_wave.pos.x + _settings.pos.x,
						_wave.pos.y + _settings.pos.y,
						_wave.pos.z + _settings.pos.z);

					if (ImGui::Button("Remove Spawn")) _removeSpawnIndex = static_cast<int>(_s);

					ImGui::PopID();
				}

				if (_removeSpawnIndex >= 0)
				{
					_wave.spawnEntities.erase(
						_wave.spawnEntities.begin() + _removeSpawnIndex);

					// 添え字がずれるのでギズモの対象は外す
					ClearGizmoTarget();
				}

				ImGui::Separator();
				if (ImGui::Button("Remove Wave")) _removeWaveIndex = static_cast<int>(_i);
			}

			ImGui::PopID();
		}

		if (_removeWaveIndex >= 0)
		{
			m_waves.erase(m_waves.begin() + _removeWaveIndex);

			// ウェーブ番号がずれるので、出し済みの個体との対応も作り直す
			ResetProgress();
			ClearGizmoTarget();
		}
	}
}
