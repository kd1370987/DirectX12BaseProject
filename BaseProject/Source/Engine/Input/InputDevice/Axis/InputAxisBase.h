#pragma once

namespace Engine::Input
{
	// 単一の軸入力状態を保持する規定クラス
	// スティックや十字操作などの、二次元入力を保持する
	class InputAxisBase
	{
	public:
		InputAxisBase() = default;
		virtual ~InputAxisBase() = default;

		virtual void PreUpdate() {};
		virtual void Update() = 0;

		// 強制的に入力をなくす
		void NoInput() { m_axis = DXSM::Vector2::Zero; }

		// アクセサ
		void SetValueRate(float a_rate) { m_valueRate = a_rate; }		// レート設定
		void SetLimitValue(float a_limit) { m_limitValue = a_limit; }	// 限界値設定

		DXSM::Vector2 GetState() const;


	protected:
		DXSM::Vector2 m_axis = {};

		// 軸の数値にかける補正
		float m_valueRate = 1.0f;

		// 軸の限界値
		float m_limitValue = FLT_MAX;
	};
}