#pragma once

namespace Engine::Graphics
{
	class RenderContext;
}

namespace Engine::Raytracing
{
	class RayWorld;
	class RayPSO;
	class ShaderTable;

	class RayEngine
	{
	public:

		// レイトレーシングをディスパッチ
		void Dispatch(Graphics::RenderContext* a_pRCT);
		void Dispatch(
			Resource::Handle<Resource::Texture> a_outHandle,
			Graphics::RenderContext* a_pRCT,
			RayPSO* a_pPSO,
			ShaderTable* a_pShaderTable
		);

		// レイトレワールドに登録
		void RegistModel(
			const DirectX::XMFLOAT4X4& a_worldMat,
			const Engine::Resource::Handle<Resource::Model>& a_modelHandle
		);

		// レイトレワールドの構築
		void CommitWorld();

		// フレーム開始処理
		void BegineFrame();
		void EndFrame();
	private:

		struct Camera
		{
			DXSM::Matrix rotMat = {};		// 回転行列
			DXSM::Vector3 pos = {};			// 座標
			float aspectRate = 0.0f;		// アスペクト比
			float farClip = 1000.0f;		// 遠平面
			float nearClip = 0.1f;			// 近平面
		};
		// GPUに送信するデータ
		Camera m_camera;

		// レイトレ用クラス
		std::unique_ptr<RayWorld> m_upRayWorld = nullptr;				// レイトレワールド
		Engine::Resource::Handle<Engine::Resource::Texture> m_outTex;	// 出力用UAVテクスチャ
		std::unique_ptr<RayPSO> m_upPSO = nullptr;						// レイトレ用PSO
		std::unique_ptr<ShaderTable> m_upShaderTable = nullptr;			// シェーダーテーブル

		bool m_isCommit = false;		// コミットされたかどうか

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