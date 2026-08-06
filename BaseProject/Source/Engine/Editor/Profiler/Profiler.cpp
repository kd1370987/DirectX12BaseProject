#include "Profiler.h"

#include "CPUProfiler/CPUProfiler.h"
#include "GPUProfiler/GPUProfiler.h"

namespace Engine::Editor
{
	Profiler::Profiler() = default;
	Profiler::~Profiler() = default;

	//======================================================================================
	// 初期化
	//======================================================================================
	void Profiler::Init()
	{
		m_upCPUProfiler = std::make_unique<CPUProfiler>();
		m_upGPUProfiler = std::make_unique<GPUProfiler>();

		m_upGPUProfiler->Init();

		m_frameCount = 0;
	}

	//======================================================================================
	// フレーム開始
	//======================================================================================
	void Profiler::BeginFrame()
	{
		if (m_upGPUProfiler)
		{
			m_upGPUProfiler->BeginFrame();
		}
	}

	//======================================================================================
	// フレーム終了
	//
	// 平均レートに達したら平均を確定させ、表示用の並べ替え済み配列を作り直す
	// パネルが読むのは常に「前フレームまでに確定した結果」になる
	//======================================================================================
	void Profiler::EndFrame()
	{
		if (m_upGPUProfiler)
		{
			m_upGPUProfiler->EndFrame();
		}

		// 平均の確定
		++m_frameCount;
		if (m_frameCount >= m_avelageRate)
		{
			m_frameCount = 0;

			if (m_upCPUProfiler)
			{
				for (auto& [_name, _timer] : m_timers)
				{
					m_upCPUProfiler->FixAverage(_timer);
				}
			}
		}

		// 表示用のスナップショットを作る
		m_results.clear();
		m_results.reserve(m_timers.size());
		for (const auto& [_name, _timer] : m_timers)
		{
			m_results.push_back({ _name, _timer });
		}

		// 平均時間の降順(重い順)に並べ替え
		std::sort(
			m_results.begin(), m_results.end(),
			[](const TimerResult& a_lhs, const TimerResult& a_rhs)
			{
				return a_lhs.timer.averageTime > a_rhs.timer.averageTime;
			}
		);
	}

	//======================================================================================
	// 計測開始
	// 未登録の名前はここで作る
	//======================================================================================
	void Profiler::StartTimer(const std::string& a_name)
	{
		if (!m_upCPUProfiler) return;

		m_upCPUProfiler->Start(m_timers[a_name]);
	}

	//======================================================================================
	// 計測終了
	//======================================================================================
	void Profiler::StopTimer(const std::string& a_name)
	{
		if (!m_upCPUProfiler) return;

		auto _it = m_timers.find(a_name);
		if (_it == m_timers.end())
		{
			ENGINE_LOG("開始していない計測を終了しようとしました : %s", a_name.c_str());
			return;
		}

		m_upCPUProfiler->Stop(_it->second);
	}

	//======================================================================================
	// リセット
	//======================================================================================
	void Profiler::ResetAll()
	{
		if (!m_upCPUProfiler) return;

		for (auto& [_name, _timer] : m_timers)
		{
			m_upCPUProfiler->Reset(_timer);
		}

		m_frameCount = 0;
	}
}
