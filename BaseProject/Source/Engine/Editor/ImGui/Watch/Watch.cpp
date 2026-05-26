#include "Watch.h"
namespace Engine::Editor
{
	void Watch::Start()
	{
		m_startTime = Clock::now();
	}

	void Watch::End()
	{
		m_endTime = Clock::now();

		m_calcTime = std::chrono::duration<double, std::milli>(m_endTime - m_startTime).count();

		m_minTime = std::min(m_minTime, m_calcTime);
		m_maxTime = std::max(m_maxTime, m_calcTime);

		// 結果用に積んでいく
		m_matchTime += m_calcTime;
		m_count++;
	}

	void Watch::DrawResult(const std::string& a_name, double a_watchTime, TimeUnit a_unit)
	{
		// 表示ボックス
		//if (ImGui::Begin(a_name.c_str()))
		{
			ImGui::Text("NowTime");
			ImGui::Text("MS : %.3f", m_calcTime);

			if (m_count > 0)
			{
				ImGui::Separator();
				ImGui::Text("Avelage : %.3f ms", m_matchTime / m_count);
				ImGui::Text("Count : %d", m_count);
			}

			ImGui::Text("Min : %.3f", m_minTime);
			ImGui::Text("Max : %.3f", m_maxTime);
		}
		//ImGui::End();
	}

	void Watch::Reset()
	{
		m_matchTime = 0.0;
		m_count = 0;
		// 平均をとる際の計測時間
		m_watchTime = 0.0;
		m_calcTime = 0.0;		// 一時結果

		m_minTime = 100000.0;
		m_maxTime = 0.0;

	}
	double Watch::GetAvelage() const
	{
		return (m_matchTime / m_count);
	}
}