#include "Engine/Graphics/Raytracing/RayWorld/RayWorld.h"

#include "Engine/Graphics/Raytracing/RayWorld/TLAS/TLAS.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"


#include "Engine/Graphics/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/ECS/World/World.h"

namespace Engine::Graphics::Raytracing
{
	Engine::Graphics::Raytracing::RayWorld::RayWorld()
	{
	}
	Engine::Graphics::Raytracing::RayWorld::~RayWorld()
	{
	}


	void Engine::Graphics::Raytracing::RayWorld::Register(
		const Math::Matrix& a_worldMat,
		const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
		const Math::Color& a_colorScale,
		const Math::Vector3& a_emissiveScale,
		const Math::Vector3& a_emissiveAdd
	)
	{
		m_isDirty = true;

		// モデルのノードとメッシュを参照してインスタンスに変換
		auto* _model = (*m_pResourceManager).Get(a_modelHandle);
		if (!_model) return;

		auto& _nodes = _model->GetOriginalNodeVec();
		for (auto& _node : _nodes)		// ノードループ
		{
			for (auto& _meshIdx : _node.meshIndices)	// メッシュループ
			{
				const auto& _meshHandle = _model->GetMeshHandles()[_meshIdx];
				const auto* _pMesh = (*m_pResourceManager).Get(_meshHandle);
				if (!_pMesh) continue;

				Math::Matrix _nodeMat = _node.worldTransform;

				// インスタンス作成
				Engine::Graphics::Raytracing::Instance _rayInst = {};
				_rayInst.worldMat = _nodeMat * a_worldMat;
				if (!_pMesh->HasRtData()) continue;
				_rayInst.pBLAS = &_pMesh->GetRtData().blas;

				// メガバッファ割り当て
				_rayInst.megaVertexHandle = _pMesh->GetRtData().vertexHandle;
				_rayInst.megaIndexHandle = _pMesh->GetRtData().indexHandle;

				for (auto& _subset : _pMesh->GetMetaData().subsets)
				{
					// マテリアル取得
					const auto& _mateHandle = _model->GetMaterialHandles()[_subset.materialNumber];
					const auto* _pMate = (*m_pResourceManager).Get(_mateHandle);
					if (!_pMate) continue;
	
					Material _mat = {};
					Math::Color _baseColor = _pMate->baseColor;
					Math::Vector3 _emiColor = _pMate->emissive;
					_mat.baseColor = _baseColor * a_colorScale;
					_mat.metallic = _pMate->metallic;
					_mat.roughness = _pMate->roughness;
					_mat.emissive = _emiColor * a_emissiveScale;
					_mat.emissiveAdd = a_emissiveAdd;
					_mat.startIndexLocation = _subset.faceStart * 3;
					_mat.baseIndex = GetTexHeapIndex(_pMate->baseColorTex);
					_mat.metaRoughnessIndex = GetTexHeapIndex(_pMate->metaRoughTex);
					_mat.emissiveIndex = GetTexHeapIndex(_pMate->emissiveTex);
					_mat.normalIndex = GetTexHeapIndex(_pMate->normalTex);

					_rayInst.submeshMaterials.push_back(_mat);
				}
				m_instanceVec.emplace_back(_rayInst);
			}
		}

		// コミットされていない状態に
		m_isCommit = false;
	}

	void RayWorld::Register(
		ECS::World& a_world,
		const Math::Matrix& a_worldMat,
		const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
		const Handle<DynamicRaytracingData>& a_dynamicDataHandle,
		const RangeHandle<Resource::NodePoseMatrix>& /*a_nodeposeMatHandle*/,
		const Math::Color& a_colorScale, 
		const Math::Vector3& a_emissiveScale,
		const Math::Vector3& a_emissiveAdd
	)
	{
		// モデルのノードとメッシュを参照してインスタンスに変換
		auto* _model = (*m_pResourceManager).Get(a_modelHandle);
		if (!_model) return;

		// アニメーション用BLASを取得
		auto& _pool = a_world.RefResource<Pool::ItemPool<DynamicRaytracingData>>();


		auto& _nodes = _model->GetOriginalNodeVec();
		for (auto& _node : _nodes)		// ノードループ
		{
			UINT _meshDataIdx = 0;
			for (auto& _meshIdx : _node.meshIndices)	// メッシュループ
			{
				const auto& _meshHandle = _model->GetMeshHandles()[_meshIdx];
				const auto* _pMesh = (*m_pResourceManager).Get(_meshHandle);
				if (!_pMesh) continue;

				// インスタンス作成
				Engine::Graphics::Raytracing::Instance _rayInst = {};
				_rayInst.worldMat = a_worldMat;
				if (!_pMesh->HasRtData()) continue;
				auto* _item = _pool.Ref(a_dynamicDataHandle);
				if (!_item) continue;

				if (_meshDataIdx >= _item->meshDataVec.size()) continue;
				_rayInst.pBLAS = &_item->meshDataVec[_meshDataIdx].instanceBLAS;

				// メガバッファの割り当て
				_rayInst.megaVertexHandle = _pMesh->GetRtData().vertexHandle;
				_rayInst.megaIndexHandle = _pMesh->GetRtData().indexHandle;

				// このインスタンスは動的(スキニング)。
				// レイの当たり判定(BLAS)だけでなくヒットシェーダの頂点属性もアニメ済みを使えるよう、
				// アニメフラグとアニメ済み頂点バッファの開始オフセットを持たせる。
				_rayInst.isAnimated = true;
				_rayInst.animatedVertexOffset =
					_item->meshDataVec[_meshDataIdx].animatedVertexHandle.startIndex;

				for (auto& _subset : _pMesh->GetMetaData().subsets)
				{
					// マテリアル取得
					const auto& _mateHandle = _model->GetMaterialHandles()[_subset.materialNumber];
					const auto* _pMate = (*m_pResourceManager).Get(_mateHandle);
					if (!_pMate) continue;

					Material _mat = {};
					Math::Color _baseColor = _pMate->baseColor;
					Math::Vector3 _emiColor = _pMate->emissive;
					_mat.baseColor			= _baseColor * a_colorScale;
					_mat.metallic			= _pMate->metallic;
					_mat.roughness			= _pMate->roughness;
					_mat.emissive			= _emiColor * a_emissiveScale;
					_mat.emissiveAdd		= a_emissiveAdd;
					_mat.startIndexLocation = _subset.faceStart * 3;
					_mat.baseIndex			= GetTexHeapIndex(_pMate->baseColorTex);
					_mat.metaRoughnessIndex = GetTexHeapIndex(_pMate->metaRoughTex);
					_mat.emissiveIndex		= GetTexHeapIndex(_pMate->emissiveTex);
					_mat.normalIndex		= GetTexHeapIndex(_pMate->normalTex);

					_rayInst.submeshMaterials.push_back(_mat);
				}
				m_instanceVec.emplace_back(_rayInst);

				_meshDataIdx++;
			}
		}
	}

	void Engine::Graphics::Raytracing::RayWorld::Init(
		Graphics::D3D12::Device* a_pDevice,
		Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
		Graphics::D3D12::GraphicsCommandList* a_pCmdList,
		Resource::ResourceManager* a_pResourceManager
	)
	{
		// ビューの置き場を控える
		m_pHeapManager = a_pHeapManager;
		m_pResourceManager = a_pResourceManager;

		// TLAS・インスタンス・マテリアルのバッファはどれも同じ上限で作る
		// レイワールド構築
		if (!m_upTLAS)
		{
			m_upTLAS = std::make_unique<TLAS>();
		}
		m_upTLAS->Create(a_pDevice, a_pHeapManager, a_pCmdList, MAX_INSTANCE_NUM);

		// インスタンスデータ作成
		m_instanceDataVec.clear();
		m_instanceDataVec.resize(MAX_INSTANCE_NUM);
		m_instanceDataBuffer.Create(a_pDevice, a_pHeapManager, a_pCmdList, MAX_INSTANCE_NUM, m_instanceDataVec.data());

		// マテリアルデータ作成
		m_materialVec.clear();
		m_materialVec.resize(MAX_INSTANCE_NUM);
		m_materialDataBuffer.Create(a_pDevice, a_pHeapManager, a_pCmdList, MAX_INSTANCE_NUM, m_materialVec.data());
	}

	void RayWorld::Release()
	{
		// インスタンスデータ解放
		m_instanceDataBuffer.Release();
		m_instanceDataVec.clear();

		// マテリアルデータ解放
		m_materialDataBuffer.Release();
		m_materialVec.clear();

		// TLAS解放
		m_upTLAS->Release();
		m_upTLAS.reset();
		m_instanceVec.clear();

	}


	void Engine::Graphics::Raytracing::RayWorld::Commit(Graphics::D3D12::GraphicsCommandList* a_pCmdList, UINT a_frameIndex)
	{
		// TLAS更新
		m_upTLAS->Update(a_pCmdList, m_instanceVec, a_frameIndex);

		UINT _materialOffset = 0;
		// 構造体バッファ更新
		m_instanceDataVec = {};
		m_materialVec = {};
		for (auto& _instance : m_instanceVec)
		{
			InstanceData _data = {};
			_data.vertexStart = _instance.megaVertexHandle.startIndex;
			_data.indexStart = _instance.megaIndexHandle.startIndex;
			_data.indexCount = _instance.megaIndexHandle.count;

			// アニメーション情報を転送(以前は未設定で常に静的頂点が使われていた)
			_data.isAnimated = _instance.isAnimated ? 1u : 0u;
			_data.animatedVertexStart = _instance.animatedVertexOffset;

			_data.materialOffset = _materialOffset;
			m_instanceDataVec.push_back(_data);

			// オフセット更新
			for (auto& _mate : _instance.submeshMaterials)
			{
				m_materialVec.push_back(_mate);
				_materialOffset++;
			}
			
		}
		// 毎フレーム丸ごと書き換えるので、フレームごとの区画を経由して送る
		// (1本のアップロードバッファを書き換えると、前フレームのコピーが読んでいる中身を踏む)
		m_instanceDataBuffer.UploadFrame(a_pCmdList, m_instanceDataVec.data(), m_instanceDataVec.size() * sizeof(InstanceData), a_frameIndex);
		m_materialDataBuffer.UploadFrame(a_pCmdList, m_materialVec.data(), m_materialVec.size() * sizeof(Material), a_frameIndex);
	}

	void Engine::Graphics::Raytracing::RayWorld::Clear()
	{
		m_instanceVec.clear();
	}

	D3D12_GPU_VIRTUAL_ADDRESS Engine::Graphics::Raytracing::RayWorld::GetTLAS() const
	{
		return m_upTLAS->GetGPUAddress();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::Graphics::Raytracing::RayWorld::GetSRVTLAS() const
	{
		return m_upTLAS->GetGPUHandle();
	}

	Handle<Graphics::D3D12::SRV> Engine::Graphics::Raytracing::RayWorld::GetInstanceBufferSRV() const
	{
		return m_instanceDataBuffer.GetSRVHandle();
	}

	Handle<Graphics::D3D12::SRV> Engine::Graphics::Raytracing::RayWorld::GetMaterialBufferSRV() const
	{
		return m_materialDataBuffer.GetSRVHandle();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::Graphics::Raytracing::RayWorld::GetInstanceDataSRV() const
	{
		return m_pHeapManager->GetGPU(m_instanceDataBuffer.GetSRVHandle());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Engine::Graphics::Raytracing::RayWorld::GetInstanceDataSRVCPU() const
	{
		return m_pHeapManager->GetCPU(m_instanceDataBuffer.GetSRVHandle());
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::Graphics::Raytracing::RayWorld::GetMaterialSRV() const
	{
		return m_pHeapManager->GetGPU(m_materialDataBuffer.GetSRVHandle());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Engine::Graphics::Raytracing::RayWorld::GetMaterialSRVCPU() const
	{
		return m_pHeapManager->GetCPU(m_materialDataBuffer.GetSRVHandle());
	}
	int RayWorld::GetTexHeapIndex(const Handle<Resource::Texture>& a_handle) const
	{
		const auto* _Ntex = (*m_pResourceManager).Get(a_handle);
		return static_cast<int>(_Ntex->GetSRV().GetIndex());
	}
}