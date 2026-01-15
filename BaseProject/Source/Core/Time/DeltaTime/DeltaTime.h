#pragma once

class DeltaTime
{
public:
	
	void CalcDeltaTime();

	float Get() const
	{
		return m_dt;
	}

private:
	float m_dt = 0.0f;

	std::chrono::steady_clock::time_point m_previousTime{};

	bool m_isFirstCalc = true;
};
