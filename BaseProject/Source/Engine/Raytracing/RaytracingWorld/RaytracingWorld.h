#pragma once

namespace Engine::Raytracing
{
	class World
	{
	public:

	private:

		struct Camera
		{
			DXSM::Matrix rotMat = {};		// 回転行列
			DXSM::Vector3 pos = {};			// 座標
			float aspectRate = 0.0f;		// アスペクト比
			float farClip = 1000.0f;		// 遠平面
			float nearClip = 0.1f;			// 近平面
		};

		Camera m_camera;			// レイトレワールドのカメラ
		std::vector<>

	};
}