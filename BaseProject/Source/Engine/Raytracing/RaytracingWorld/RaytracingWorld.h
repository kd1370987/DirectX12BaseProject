#pragma once

namespace Engine::Raytracing
{
	struct Instance
	{
		DirectX::XMFLOAT4X4 worldMat = DXSM::Matrix::Identity;
		BLAS* pBLAS = nullptr;
	};

	class World
	{
	public:

		// モデルとワールド行列を登録して内部でインスタンスに返還
		void Register(
			const DirectX::XMFLOAT4X4& a_worldMat,
			const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle
		);

	private:

		struct Camera
		{
			DXSM::Matrix rotMat = {};		// 回転行列
			DXSM::Vector3 pos = {};			// 座標
			float aspectRate = 0.0f;		// アスペクト比
			float farClip = 1000.0f;		// 遠平面
			float nearClip = 0.1f;			// 近平面
		};

		Camera m_camera;								// レイトレワールドのカメラ
		std::vector<Instance> m_instanceVec = {};		// レイトレワールドインスタンス
	};
}