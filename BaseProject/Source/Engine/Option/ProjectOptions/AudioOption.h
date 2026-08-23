#pragma once

#include "../IOption.h"

#include "../../Audio/SoundGroup.h"

namespace Engine::Option::ProjectOptions
{
	/// <summary>
	/// 音量の設定
	/// </summary>
	/// <remarks>
	/// 実際に鳴る音量は
	///     鳴らす側が指定した音量 × グループ音量 × マスター音量
	/// で決まる。ここが持つのは後ろの2つ。
	///
	/// グループ(BGM / SE / UI / Voice)は発行時に音へ付ける札で、
	/// ここを動かせば、そのグループの音を鳴らしている側を1つも触らずに
	/// まとめて上下できる。
	///
	/// 値は AudioManager が実行中の持ち主なので、
	/// 読み込んだ後とエディターで動かした後に Apply() で流し込む。
	/// 保存側と実行側を分けてあるのは、AudioManager がオプションを
	/// 名指しで引きに行かなくて済むようにするため。
	/// </remarks>
	struct AudioOption : IOption
	{
		// 全体へ掛かる音量
		float masterVolume = 1.0f;

		// グループごとの音量。並びは Audio::ESoundGroup と同じ
		std::array<float, Audio::SOUND_GROUP_COUNT> groupVolumeArray = { 1.0f, 1.0f, 1.0f, 1.0f };

		// 音量を AudioManager へ流し込む(鳴っている音にもその場で効く)
		void Apply() const;

		const std::string& GetName() override
		{
			static const std::string _name = "AudioOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Project;
		}

		void DrawEdit() override;
		void Archive(Persistence::Archive& a_archive) override;
	};
}
