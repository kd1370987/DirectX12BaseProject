#pragma once
#include "../../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	/// <summary>
	/// 1体ぶんの出現設定
	/// </summary>
	struct SpowanSettings
	{
		// 出現させるエンティティ
		Engine::GUID								spowanEntityGUID = {};		// 出すプレハブ(保存する)
		Engine::Handle<Engine::Resource::Prefab>	spowanPrefabHandle = {};	// 解決済みハンドル(ランタイム)

		// 初期情報
		DXSM::Vector3 pos = {};						// 位置
		DXSM::Vector3 dir = { 0.0f, 0.0f, 1.0f };	// 方向(左手系 +Z 前方。水平成分だけ使う)
	};

	/// <summary>
	/// エンティティを出現させる仕様
	/// </summary>
	struct Weave
	{
		// 出現させるエンティティ
		std::vector<SpowanSettings> spowanEntities = {};

		// ウェーブ条件(保存する)
		float timing = 0.0f;			// 出現タイミング(秒)
										//   isAnnihilation = false : シーン開始からの経過時間
										//   isAnnihilation = true  : 前のウェーブが全滅してからの経過時間
		bool isAnnihilation = false;	// 前のウェーブが全滅してから出現させるかどうか

		// 現在の状態(保存しない。シーン開始時は既定値から始まる)
		bool  isSpowan    = false;		// 出現処理を済ませたか
		int   spowanCount = 0;			// 出した数(生成コマンドを積めた数)
		int   aliveCount  = 0;			// 生存数。毎フレーム SpownerComponent を数え直す
		bool  isConfirmed = false;		// 実体化を一度でも確認したか(遅延生成の1フレームずれ対策)
		bool  isCleared   = false;		// 全滅したか
		float spowanTime  = 0.0f;		// 出現させた時刻(シーン経過秒)
		float clearedTime = 0.0f;		// 全滅した時刻(シーン経過秒)
	};

	/// <summary>
	/// シーン全体の流れを握る : シーンに一つのみ存在
	///
	/// 敵の出現(ウェーブ)やチュートリアルの操作説明など、
	/// 「時間や状況で進むイベント」をここでまとめて管理する。
	/// 今はウェーブだけを持つ。
	///
	/// ・出現させたエンティティには SpownerComponent(自分のGUID + ウェーブ番号)が付く。
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

	private:

		// 出現位置の印をデバッグ描画する
		void DrawSpowanMarker(Engine::GameObject::ObjectContext& a_context) const;

		// SpownerComponent を数えて各ウェーブの生存数を更新する
		void UpdateAliveCount(Engine::GameObject::ObjectContext& a_context);

		// 全滅の判定を進める
		void UpdateClearState();

		// 出現条件を満たしているか
		bool CanSpowan(size_t a_index) const;

		// ウェーブを出現させる
		void Spowan(Engine::GameObject::ObjectContext& a_context, size_t a_index);

		// 進行状況を初期化して最初からやり直す
		void ResetProgress();

	private:

		// 敵の出現をつかさどる
		std::vector<Weave> m_weaves = {};

		// シーンの経過時間
		float m_time = 0.0f;
	};
}
