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

		// コミット
		void Commit();
		void BindCamera(Graphics::RenderContext* a_pRCT);
		void BindTLAS(Graphics::RenderContext* a_pRCT);
		void Dispatch(Graphics::RenderContext* a_pRCT, ShaderTable& a_shadertable);

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

		// インスタンス配列取得
		const std::vector<Instance>& GetInstanceVec();
	private:

		struct Camera
		{

			DirectX::XMFLOAT3 pos;						// カメラ座標
			float pad;
			DirectX::XMFLOAT4X4 view;					// ビュー行列
			DirectX::XMFLOAT4X4 proj;					// プロジェクション行列

			DirectX::XMFLOAT4X4 invView;				// 逆ビュー行列
			DirectX::XMFLOAT4X4 invProj;				// 逆プロジェクション行列

			DirectX::XMFLOAT4X4 invViewProj;			// 逆ビュープロジェクション行列
		};
		// GPUに送信するデータ
		Camera m_camera;

		// レイトレ用クラス
		std::unique_ptr<RayWorld> m_upRayWorld = nullptr;				// レイトレワールド
		

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