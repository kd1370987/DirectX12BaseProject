#pragma once

#include "../InputAxisBase.h"

namespace Engine::Input
{
	class InputButtonBase;

	/// <summary>
	/// WinAPIのGetAsyncKeyStateの入力を利用した軸制御
	/// 指定した上下左右のキーの入力状況を軸情報として保持する
	/// </summary>
	class InputAxisForWindows : public InputAxisBase
	{
	public:

		InputAxisForWindows(int a_upCode, int a_rightCode, int a_downCode, int a_leftCode);
		~InputAxisForWindows() override;

		void PreUpdate() override;
		void Update(InputContext& a_inputContext) override;

	private:

		enum class EDir
		{
			Up,
			Right,
			Down,
			Left,
			Max
		};

		// 方向を配列の添え字にする
		static constexpr size_t ToIndex(EDir a_dir) { return static_cast<size_t>(a_dir); }

	private:

		// 方向ごとのボタン(EDir の順)
		std::array<std::unique_ptr<InputButtonBase>, static_cast<size_t>(EDir::Max)> m_upDirButtons;
	};
}