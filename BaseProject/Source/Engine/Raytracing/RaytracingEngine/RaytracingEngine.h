#pragma once
#include "../../Graphics/CBData.h"

namespace Engine
{
	namespace Graphics
	{
		class RenderContext;
		class GraphicsEngine;
	}

	namespace ECS
	{
		class World;
	}
}

namespace Engine::Raytracing
{
	class RayWorld;
	class RayPSO;
	class ShaderTable;

	class RayEngine
	{
	public:

		// 解放
		void Release();

		// コミット
		void Commit(D3D12::GraphicsCommandList* a_pCmdList);
		void BindCamera(Graphics::RenderContext* a_pRCT,const Graphics::CameraData& a_cbCam);
		void BindTLAS(Graphics::RenderContext* a_pRCT);
		void Dispatch(Graphics::RenderContext* a_pRCT, ShaderTable& a_shadertable);

		// レイトレワールドに登録
		void RegistModel(
			const DirectX::XMFLOAT4X4& a_worldMat,
			const Engine::Handle<Resource::Model>& a_modelHandle,
			const DXSM::Vector4& a_colorScale,
			const DXSM::Vector3& a_emissiveScale
		);
		void RegisterSkinningModel(
			ECS::World& a_world,
			const DXSM::Matrix& a_worldMat,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			const Handle<DynamicRaytracingData>& a_dynamicData,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodeposeMatVec,
			const DXSM::Vector4& a_colorScale,
			const DXSM::Vector3& a_emissiveScale
		);
		// レイトレワールドの構築
		void CommitWorld(D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList);

		// フレーム開始処理
		void BeginFrame();
		void EndFrame();

		// インスタンス配列取得
		const std::vector<Instance>& GetInstanceVec();
	private:

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