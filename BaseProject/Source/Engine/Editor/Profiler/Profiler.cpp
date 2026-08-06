#include "Profiler.h"

#include "CPUProfiler/CPUProfiler.h"
#include "GPUProfiler/GPUProfiler.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

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
	// 解放
	// リードバックバッファをマップしっぱなしにしているので、デバイス破棄より前に外す
	//======================================================================================
	void Profiler::Release()
	{
		if (m_upGPUProfiler)
		{
			m_upGPUProfiler->Releasse();
		}
	}

	//======================================================================================
	// フレーム開始
	//
	// GPUクエリの割り当てを先頭に戻すだけ
	// このフレームで積まれる GPU計測より前に呼ばれている必要がある
	//======================================================================================
	void Profiler::BeginFrame()
	{
		if (m_upGPUProfiler)
		{
			m_upGPUProfiler->BeginFrame();
		}
	}

	//======================================================================================
	// GPU計測結果の読み戻し
	//
	// タイムスタンプはコマンドリストがGPUで実行されて初めて確定するので、
	// フレーム末尾ではなく「GPU待機を抜けた直後」に呼ぶ
	// 読めるのは待機が保証しているフレームぶんなので、数フレーム前の結果になる
	//======================================================================================
	void Profiler::CollectGPUResult()
	{
		if (!m_upGPUProfiler) return;

		// GetDirectCommandList() はプールからリストを確保する副作用付きなので呼ばない
		// (読み戻しにコマンドリストは不要)
		m_upGPUProfiler->EndFrame(
			D3D12::D3D12Wrapper::Instance().GetCommandQueue(),
			m_timers
		);
	}

	//======================================================================================
	// フレーム終了
	//
	// 平均レートに達したら平均を確定させ、表示用の並べ替え済み配列を作り直す
	// パネルが読むのは常に「前フレームまでに確定した結果」になる
	//======================================================================================
	void Profiler::EndFrame()
	{
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
	void Profiler::StartTimer(const std::string& a_name, D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!m_upCPUProfiler) return;

		Timer& _timer = m_timers[a_name];

		m_upCPUProfiler->Start(_timer);

		// コマンドリストがあればGPU計測もする
		if (!a_pCmdList) return;
		if (!m_upGPUProfiler) return;

		m_upGPUProfiler->Start(a_pCmdList, _timer);
	}

	//======================================================================================
	// 計測終了
	//======================================================================================
	void Profiler::StopTimer(const std::string& a_name, D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!m_upCPUProfiler) return;

		auto _it = m_timers.find(a_name);
		if (_it == m_timers.end())
		{
			ENGINE_LOG("開始していない計測を終了しようとしました : %s", a_name.c_str());
			return;
		}

		m_upCPUProfiler->Stop(_it->second);

		// コマンドリストがあればGPU計測もする
		if (!a_pCmdList) return;
		if (!m_upGPUProfiler) return;

		m_upGPUProfiler->End(a_pCmdList, _it->second);
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
