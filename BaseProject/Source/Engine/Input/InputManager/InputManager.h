#pragma once

namespace Engine::Input
{
	class InputCollector;

	// 様々な入力を管理するクラス : 複数のInputCllectorを管理
	class InputManager
	{
	public:

		// 更新
		// 毎フレーム必須
		void Update();

		// すべての有効な入力装置からのボタン入力状態を取得
		short GetButtonState(std::string_view a_name) const;

		bool IsFree(std::string_view a_name) const;
		bool IsPress(std::string_view a_name) const;
		bool IsHold(std::string_view a_name) const;
		bool IsRelease(std::string_view a_name) const;

		// すべての有効な入力装置からの軸入力状態を取得
		DXSM::Vector2 GetAxisState(std::string_view a_name) const;

		// 入力装置の登録
		void AddDevice(std::string_view a_name,InputCollector* a_pInputDevice);
		void AddDevice(std::string_view a_name,std::unique_ptr<InputCollector> a_upInputDevice);

		// アクセサ
		const std::unique_ptr<InputCollector>& GetDevice(std::string_view a_name) const;
		std::unique_ptr<InputCollector>& RefDevice(std::string_view a_name);

		// 解放
		void Release();

	private:

		std::unordered_map<std::string, std::unique_ptr<InputCollector>> m_upInputDeviceMap = {};

	private:
		InputManager();
		~InputManager();
	public:

		static InputManager& Instance()
		{
			static InputManager _instance;
			return _instance;
		}
	};
}