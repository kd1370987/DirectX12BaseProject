#pragma once

namespace Engine::Raytracing
{
	class RayWorld;

	class RayEngine
	{
	public:

		void Create();

		// レイトレーシングをディスパッチ
		void Dispatch();

		// レイトレワールドに登録
		void RegistModel(
			const DirectX::XMFLOAT4X4& a_worldMat,
			const Engine::Resource::Handle<Resource::Model>& a_modelHandle
		);

		// レイトレワールドの構築
		void CommitWorld();
	private:

		struct Camera
		{
			DXSM::Matrix rotMat = {};		// 回転行列
			DXSM::Vector3 pos = {};			// 座標
			float aspectRate = 0.0f;		// アスペクト比
			float farClip = 1000.0f;		// 遠平面
			float nearClip = 0.1f;			// 近平面
		};

		Camera m_camera;

		// レイトレワールド
		std::unique_ptr<RayWorld> m_upRayWorld = nullptr;

		// 出力用UAVテクスチャ
		Engine::Resource::Handle<Engine::Resource::Texture> m_outTex;

	private:

		RayEngine();
		~RayEngine();
		
	public:

		static RayEngine& Instance()
		{
			static RayEngine _instance;
			return _instance;
		}

	};
}