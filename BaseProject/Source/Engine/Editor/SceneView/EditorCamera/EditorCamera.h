#pragma once
namespace Engine::Editor
{
	class EditorCamera
	{
	public:

		void Init(UINT a_windowWidth,UINT a_windowHeight);

		void Update();

		// アクセサ
		DXSM::Matrix GetWorldMat() const { return m_worldMat; }	// ワールド行列
		DXSM::Matrix GetProjMat() const { return m_projMat; }		// 射影行列

	private:

		// カメラステータス
		DXSM::Vector3 m_pos;
		DXSM::Vector3 m_rot;
		DXSM::Vector3 m_scale;

		DXSM::Matrix m_worldMat = {};
		
		float m_fov = 60.0f;				// 対直視野角
		float m_aspectRatio = 16.0f / 9.0f;	// アスペクト比
		float m_nearZ = 0.1f;				// ニアクリップ距離
		float m_farZ = 1000.0f;				// ファークリップ距離
		
		float focusDistance = 0.0f;			// 焦点距離
		float forcusRange = 0.0f;			// 焦点範囲
		float forcusBackRange = 1000.0f;	// 焦点後ろ範囲

		DXSM::Matrix m_projMat = {};	    // 射影行列
		DXSM::Matrix m_projInvMat = {};		// 射影逆行列

	};
}