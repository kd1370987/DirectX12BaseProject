#include "TimeManager.h"

#include "Core/Time/FPSController/FPSController.h"
#include "Core/Time/DeltaTime/DeltaTime.h"
namespace Engine::Time
{
	TimeManager::TimeManager()
	{}

	TimeManager::~TimeManager()
	{}

	void TimeManager::Init(float a_targetFPS)
	{
		timeBeginPeriod(1);		// Windowsのタイマー精度を1msに設定

		m_upFPSController = std::make_unique<FPSController>();
		m_upFPSController->SetMaxFPS(static_cast<UINT>(a_targetFPS));

		m_upDeltaTime = std::make_unique<DeltaTime>();
	}

	void TimeManager::Release()
	{
		timeEndPeriod(1);		// Windowsのタイマー精度を元に戻す
	}

	void TimeManager::BeginFrame()
	{
		m_upFPSController->BeginFrame();
		m_upDeltaTime->CalcDeltaTime();
	}

	void TimeManager::EndFrame(bool a_isVsync)
	{
		m_upFPSController->EndFrame(a_isVsync);
	}

	float TimeManager::GetDeltaTime() const
	{
		return m_upDeltaTime->Get();
	}

	UINT TimeManager::GetNowFPS() const
	{
		return m_upFPSController->GetNowFPS();
	}
}