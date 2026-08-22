#pragma once
namespace App::Game
{
	/// <summary>
	/// プレイの結末
	/// </summary>
	enum class EGameResult : uint32_t
	{
		None,		// まだ決着していない(プレイ中)
		Clear,		// 生き残ってウェーブを出し切った
		GameOver,	// プレイヤーが倒された
	};

	/// <summary>
	/// シーンをまたいで受け取るデータ
	///
	/// 例) メニューで選んだ戦闘ステージ、機体情報、ゲームシーンでのタイム、スコア
	/// </summary>
	/// <remarks>
	/// ワールド(ECS)のリソースはシーンを切り替えると作り直されるので、
	/// リザルトへ持っていきたいものはここへ集める。
	/// 実体は GameManager が1つだけ持っていて、
	/// App::Game::GameManager::Instance().RefGameData() で触る。
	///
	/// 書く側 : ScoreSystem(スコア) / SceneSequence(タイム・結末・ウェーブ)
	/// 読む側 : ScoreHUD、リザルトの表示物
	/// </remarks>
	struct GlobalGameContext
	{
		// ---- ゲームシーンからリザルトへ引き継がれる ----
		int   score = 0;			// ゲーム内スコア(倒した相手の ScoreTargetComponent ぶん)
		int   killCount = 0;		// 倒した数
		float time = 0.0f;			// クリアもしくは死亡時のタイム(秒)

		EGameResult result = EGameResult::None;	// どう終わったか

		int clearedWaveCount = 0;	// 出し切ったウェーブ数(ボスのウェーブも1つとして数える)
		int totalWaveCount   = 0;	// ウェーブの総数

		/// <summary>
		/// 1プレイぶんの記録を消す
		/// </summary>
		/// <remarks>
		/// ゲームシーンが始まったときに SceneSequence が呼ぶ。
		/// リザルトからやり直しても前回のスコアが残らないようにするため。
		/// </remarks>
		void ResetRun()
		{
			score = 0;
			killCount = 0;
			time = 0.0f;
			result = EGameResult::None;
			clearedWaveCount = 0;
			totalWaveCount = 0;
		}

		/// <summary>
		/// スコアを加算する。0 以下は数えるだけで足さない
		/// </summary>
		void AddScore(int a_score)
		{
			++killCount;

			if (a_score <= 0) return;
			score += a_score;
		}
	};
}
