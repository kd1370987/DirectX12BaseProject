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

namespace Engine::D3D12
{
	class DescriptorHeapManager;
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
			const Math::Matrix& a_worldMat,
			const Engine::Handle<Resource::Model>& a_modelHandle,
			const Math::Color& a_colorScale,
			const Math::Vector3& a_emissiveScale,
			const Math::Vector3& a_emissiveAdd = { 0,0,0 }
		);
		void RegisterSkinningModel(
			ECS::World& a_world,
			const Math::Matrix& a_worldMat,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			const Handle<DynamicRaytracingData>& a_dynamicData,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodeposeMatVec,
			const Math::Color& a_colorScale,
			const Math::Vector3& a_emissiveScale,
			const Math::Vector3& a_emissiveAdd = { 0,0,0 }
		);
		// レイトレワールドの構築
		void CommitWorld(D3D12::Device* a_pDevice,
			D3D12::DescriptorHeapManager* a_pHeapManager,
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