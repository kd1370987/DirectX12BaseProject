#include "EditorCamera.h"
namespace Engine::Editor
{
	void EditorCamera::Init(UINT a_windowWidth, UINT a_windowHeight)
	{
		m_worldMat = DXSM::Matrix::Identity;
		m_aspectRatio = (float)a_windowWidth / (float)a_windowHeight;
		m_fov = 60.0f;
		m_nearZ = 0.1f;
		m_farZ = 1000.0f;

		m_projMat = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(m_fov),
			m_aspectRatio,
			m_nearZ,
			m_farZ
		);
		m_projInvMat = m_projMat.Invert();
	}
	
	void EditorCamera::Update()
	{
		DXSM::Matrix _transMat = DXSM::Matrix::CreateTranslation(m_pos);
		DXSM::Matrix _rotMat = 
			DXSM::Matrix::CreateRotationX(m_rot.x) * DXSM::Matrix::CreateRotationY(m_rot.y) * DXSM::Matrix::CreateRotationZ(m_rot.z);
		DXSM::Matrix _scaleMat = DXSM::Matrix::CreateScale(m_scale);

		DXSM::Matrix _mat = _transMat * _rotMat * _scaleMat;

		m_worldMat = _mat;
	}
}