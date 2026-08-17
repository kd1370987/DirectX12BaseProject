#pragma once
#include "../../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	/// <summary>
	/// 1体ぶんの出現設定
	/// </summary>
	struct SpawnSettings
	{
		// 出現させるエンティティ
		Engine::GUID								spawnEntityGUID = {};		// 出すプレハブ(保存する)
		Engine::Handle<Engine::Resource::Prefab>	spawnPrefabHandle = {};	// 解決済みハンドル(ランタイム)

		// 初期情報
		Math::Vector3 pos = {};						// 位置(ウェーブ位置からの相対座標)
		Math::Vector3 dir = { 0.0f, 0.0f, 1.0f };	// 方向(左手系 +Z 前方。水平成分だけ使う)
	};

	/// <summary>
	/// エンティティを出現させる仕様
	/// </summary>
	struct Wave
	{
		// 出現させるエンティティ
		std::vector<SpawnSettings> spawnEntities = {};

		// ウェーブの基準位置(ワールド)。
		// 各 SpawnSettings.pos はここからの相対座標なので、ここを動かせば
		// そのウェーブの出現位置がまとめて動く。
		Math::Vector3 pos = {};

		// ウェーブ条件(保存する)
		float timing = 0.0f;			// 出現タイミング(秒)
										//   isAnnihilation = false : シーン開始からの経過時間
										//   isAnnihilation = true  : 前のウェーブが全滅してからの経過時間
		bool isAnnihilation = false;	// 前のウェーブが全滅してから出現させるかどうか

		// 現在の状態(保存しない。シーン開始時は既定値から始まる)
		bool  isSpawned   = false;		// 出現処理を済ませたか
		int   spawnCount  = 0;			// 出した数(生成コマンドを積めた数)
		int   aliveCount  = 0;			// 生存数。毎フレーム SpawnerComponent を数え直す
		bool  isConfirmed = false;		// 実体化を一度でも確認したか(遅延生成の1フレームずれ対策)
		bool  isCleared   = false;		// 全滅したか
		float spawnTime   = 0.0f;		// 出現させた時刻(シーン経過秒)
		float clearedTime = 0.0f;		// 全滅した時刻(シーン経過秒)
	};

	/// <summary>
	/// シーン全体の流れを握る : シーンに一つのみ存在
	///
	/// 敵の出現(ウェーブ)やチュートリアルの操作説明など、
	/// 「時間や状況で進むイベント」をここでまとめて管理する。
	/// 今はウェーブだけを持つ。
	///
	/// ・出現させたエンティティには SpawnerComponent(自分のGUID + ウェーブ番号)が付く。
	///   全滅判定は毎フレームその印を数えるだけなので、エンティティIDを持ち歩かなくて済む
	///   (プレハブの実体化は遅延生成なので、出した直後はIDが取れない)。
	/// ・生成は GameObjectManager::Update から呼ばれる。ECSのシステム反復の外なので
	///   World の遅延生成コマンドがそのまま次の BeginFrame で消化される。
	/// </summary>
	class SceneSequence : public Engine::GameObject::BaseObject
	{
	public:

		// 更新処理 : 経過時間を進め、条件を満たしたウェーブを出す
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "SceneSequence"; }

		// インスペクター : ウェーブの編集と進行状況の表示
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

		// シーンビューのギズモ : インスペクタで選んだ位置(ウェーブ or 出現位置)を動かす
		bool DrawGizmo(
			const Engine::GameObject::ObjectGizmoContext& a_ctx,
			Engine::GameObject::ObjectContext& a_context) override;

	private:

		// 出現位置の印をデバッグ描画する
		void DrawSpawnMarker(Engine::GameObject::ObjectContext& a_context) const;

		// SpawnerComponent を数えて各ウェーブの生存数を更新する
		void UpdateAliveCount(Engine::GameObject::ObjectContext& a_context);

		// 全滅の判定を進める
		void UpdateClearState();

		// 出現条件を満たしているか
		bool CanSpawn(size_t a_index) const;

		// ウェーブを出現させる
		void Spawn(Engine::GameObject::ObjectContext& a_context, size_t a_index);

		// 進行状況を初期化して最初からやり直す
		void ResetProgress();

		// ギズモの対象を外す(要素を消して添え字がずれたとき用)
		void ClearGizmoTarget();

	private:

		// 敵の出現をつかさどる
		std::vector<Wave> m_waves = {};

		// シーンの経過時間
		float m_time = 0.0f;

		//=======================================================================
		// エディター用(保存しない)
		//=======================================================================
		// ギズモで動かす対象。ImGuizmo は一度に1つしか操作できないので、
		// インスペクタで選んだ1点だけを出す。
		//   m_gizmoWaveIndex  < 0 … ギズモを出さない
		//   m_gizmoSpawnIndex < 0 … ウェーブの基準位置を動かす(配下がまとめて動く)
		//   それ以外              … その出現位置(ウェーブからの相対)を動かす
		int m_gizmoWaveIndex  = -1;
		int m_gizmoSpawnIndex = -1;
	};
}
