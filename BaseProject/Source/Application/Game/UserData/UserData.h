#pragma once
#include "../Core/InputSettings.h"

namespace App::Game
{
	class UserData
	{
	public:

		// 各ステージごとの記録
		struct StageRecord
		{
			bool cleard = false;
			float bestTime = FLT_MAX;
			uint32_t bestScore = 0;
		};

		// ---- 音量設定 ----
		struct SoundSettings
		{
			// マスター音量
			uint32_t mainVol = 50;

			// 各種音量
			uint32_t bgmVol = 100;
			uint32_t uiVol = 100;
			uint32_t seVol = 100;
		};

		

		// ユーザーデータすべてのアーカイブ処理
		void Archive(Engine::Persistence::Archive& a_ar);

		// アクセサ
		SoundSettings& RefDoundSettings() { return m_soundSettings; }
		InputSettings& RefInputSettings() { return m_inputSettings; }
		const InputSettings& GetInputSettings() const { return m_inputSettings; }

	private:

		// ゲームデータ
		void GameDataArchive(Engine::Persistence::Archive& a_ar);

		// サウンド設定
		void SoundArchive(Engine::Persistence::Archive& a_ar);

		// 入力設定データ
		void InputArchive(Engine::Persistence::Archive& a_ar);

	private:

		SoundSettings m_soundSettings;
		InputSettings m_inputSettings;

	};
}