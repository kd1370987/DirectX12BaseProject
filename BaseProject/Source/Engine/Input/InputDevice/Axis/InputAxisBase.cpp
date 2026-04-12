#include "InputAxisBase.h"

DXSM::Vector2 Engine::Input::InputAxisBase::GetState() const
{
	DXSM::Vector2 _retAxis = m_axis * m_valueRate;

	_retAxis.x = std::clamp(_retAxis.x, -m_limitValue, m_limitValue);
	_retAxis.y = std::clamp(_retAxis.y, -m_limitValue, m_limitValue);

	return _retAxis;
}
