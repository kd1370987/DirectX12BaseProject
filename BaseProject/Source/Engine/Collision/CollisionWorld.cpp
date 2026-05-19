#include "CollisionWorld.h"
namespace Engine::Collision
{

	// ビルド用の中間データ
	struct BuildInstance
	{
		int originalIndex = 0;
		DirectX::BoundingBox modelBoundingBox = {};
	};

	// TLAS構築用関数
	int BuildBVHInternal(
		const std::vector<CollisionInstance>& a_instanceVec,
		std::vector<Resource::BVHNode>& a_outNodeVec,
		std::vector<int>& a_outIndexVec,
		std::vector<BuildInstance>& a_buildBoxes,
		int a_start,
		int a_count,
		const int a_maxBoxPerLeaf = 4 // 一つの葉ノードに入れる最大インスタンス数
	)
	{
		// ノードの追加
		Resource::BVHNode _node;
		int _nodeIdx = static_cast<int>(a_outNodeVec.size());
		a_outNodeVec.push_back(_node);

		// このノードに含まれる全ボックスを計算
		std::vector<DirectX::XMFLOAT3> _points;
		_points.reserve(a_count * 8);
		for (int _i = 0; _i < a_count; ++_i)
		{
			int _idx = a_buildBoxes[a_start + _i].originalIndex;
			const auto& _box = a_instanceVec[_idx].worldAABB;
		}
	}

	Resource::Handle<CollisionInstance> Engine::Collision::CollisionWorld::AllcateStaticEntity(
		const CollisionInstance& a_instance
	)
	{
		// 割り当てられたハンドルのインデックスを見て配列のサイズを変更する
		auto _handle = m_staticHandleStorage.Allocate();
		if (_handle.idx > m_staticInstanceVec.size())
		{
			m_staticInstanceVec.resize(_handle.idx + 1);
		}

		// インスタンス配列に追加
		m_staticInstanceVec[_handle.idx] = a_instance;

		// ビルド実行予定
		m_isStaticDirty = true;
		
		return _handle;
	}

	Resource::Handle<CollisionInstance> CollisionWorld::AllcateDynamicEntity(const CollisionInstance& a_instance)
	{
		auto _handle = m_dynamicHandleStorage.Allocate();
		if (_handle.idx > m_dynamicInstanceVec.size())
		{
			m_dynamicInstanceVec.resize(_handle.idx + 1);
		}

		// インスタンス配列に追加
		m_dynamicInstanceVec[_handle.idx] = a_instance;

		return _handle;
	}

	void CollisionWorld::BuildWorld(UINT a_resizeNum)
	{
		// 変更があればワールドの構築をし直す
		if (m_isStaticDirty)
		{
			const UINT _instanceCount = m_staticInstanceVec.size();

			// インデックス配列のリセット
			m_staticInstanceIndexVec.clear();
			m_staticInstanceIndexVec.reserve(_instanceCount);
			
			// ノード配列のリセット
			m_staticNodeVec.clear();
			m_staticNodeVec.reserve(_instanceCount);

			// ビルド用データの作成
			std::vector<BuildInstance> _buildInstanceVec = {};
			_buildInstanceVec.resize(_instanceCount);
			for (size_t _i = 0; _i < _instanceCount; ++_i)
			{
				const auto& _originData = m_staticInstanceVec[_i];

				_buildInstanceVec[_i].originalIndex = _i;
				_buildInstanceVec[_i].modelBoundingBox = _originData.worldAABB;
			}

			m_staticRootNodeIndex = ;

			// ワールド全体のローカルAABBはルートノードのAABBと同じになる
			if (!m_staticNodeVec.empty())
			{
				m_worldAABB = m_staticNodeVec[m_staticRootNodeIndex].box;
			}

			// 更新完了
			m_isStaticDirty = false;
		}
	}
	void CollisionWorld::Clear()
	{
	}
}