#include "CollisionWorld.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

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

		// 既存の箱に次の箱をマージしてAABBを作成する
		DirectX::BoundingBox _nodeBox = a_instanceVec[a_buildBoxes[a_start].originalIndex].worldAABB;
		for (int _i = 1; _i < a_count; ++_i)
		{
			int _idx = a_buildBoxes[a_start + _i].originalIndex;
			DirectX::BoundingBox::CreateMerged(_nodeBox,_nodeBox,a_instanceVec[_idx].worldAABB);
		}
		a_outNodeVec[_nodeIdx].box = _nodeBox;

		// ボックスが閾値以下なら葉ノードにする
		if (a_count <= a_maxBoxPerLeaf)
		{
			a_outNodeVec[_nodeIdx].leftChild = -1;
			a_outNodeVec[_nodeIdx].rightChild = -1;
			a_outNodeVec[_nodeIdx].dataStart = static_cast<int>(a_outIndexVec.size());
			a_outNodeVec[_nodeIdx].dataCount = a_count;

			// 葉ノードが参照するデータインデックスを確定させる
			for (int _i = 0; _i < a_count; ++_i)
			{
				a_outIndexVec.push_back(a_buildBoxes[a_start + _i].originalIndex);
			}
			return _nodeIdx;
		}

		// 枝ノードの場合
		// もっとも広がっている軸を見つける
		const auto& _extents = a_outNodeVec[_nodeIdx].box.Extents;
		int _axis = 0;		// 0 : x , 1 : y , 2 : z
		if (_extents.y > _extents.x && _extents.y > _extents.z) _axis = 1;
		if (_extents.z > _extents.x && _extents.z > _extents.y) _axis = 2;

		// 選んだ軸の座標で三角形をソートする
		std::sort(a_buildBoxes.begin() + a_start, a_buildBoxes.begin() + a_start + a_count,
			[_axis](const BuildInstance& a, const BuildInstance& b)
			{
				if (_axis == 0) return a.modelBoundingBox.Center.x < b.modelBoundingBox.Center.x;
				if (_axis == 1) return a.modelBoundingBox.Center.y < b.modelBoundingBox.Center.y;
				return a.modelBoundingBox.Center.z < b.modelBoundingBox.Center.z;
			}
		);

		// 中央値で分割して再帰ビルド
		int _mid = a_count / 2;

		// 左側ビルド
		int _leftChildIndex = BuildBVHInternal(
			a_instanceVec,a_outNodeVec,a_outIndexVec,a_buildBoxes,a_start,_mid,a_maxBoxPerLeaf);
		// 右側ビルド
		int _rightChildIndex = BuildBVHInternal(
			a_instanceVec, a_outNodeVec, a_outIndexVec, a_buildBoxes, a_start + _mid, a_count - _mid, a_maxBoxPerLeaf);

		// 親ノードに子供のインデックスを指定
		a_outNodeVec[_nodeIdx].leftChild = _leftChildIndex;
		a_outNodeVec[_nodeIdx].rightChild = _rightChildIndex;

		return _nodeIdx;
	}

	Resource::Handle<CollisionInstance> Engine::Collision::CollisionWorld::AllcateStaticEntity(
		const CollisionInstance& a_instance
	)
	{
		// 割り当てられたハンドルのインデックスを見て配列のサイズを変更する
		auto _handle = m_staticHandleStorage.Allocate();
		if (_handle.idx >= m_staticInstanceVec.size())
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
		if (_handle.idx >= m_dynamicInstanceVec.size())
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
			_buildInstanceVec.reserve(_instanceCount);
			for (size_t _i = 0; _i < _instanceCount; ++_i)
			{
				const auto& _originData = m_staticInstanceVec[_i];

				// 有効なエンティティのみビルド対象にする
				if (_originData.entity == ECS::Limits::INVALID_ENTITY) continue;

				BuildInstance _bi = {};
				_bi.originalIndex = _i;
				_bi.modelBoundingBox = _originData.worldAABB;
				_buildInstanceVec.push_back(_bi);
			}

			// 実際に存在する有効な数でのビルド
			if(_buildInstanceVec.size() > 0)
			{
				// ノードの再帰ビルド開始
				m_staticRootNodeIndex = BuildBVHInternal(
					m_staticInstanceVec, m_staticNodeVec, m_staticInstanceIndexVec, _buildInstanceVec, 0, _instanceCount
				);

				// ワールド全体のローカルAABBはルートノードのAABBと同じになる
				if (!m_staticNodeVec.empty())
				{
					m_worldAABB = m_staticNodeVec[m_staticRootNodeIndex].box;
				}
			}

			// 更新完了
			m_isStaticDirty = false;
		}

		// デバッグ描画
		auto* _pRCT = Engine::MainEngine::Instance().RefRenderContext();
		for (auto& _node : m_staticNodeVec)
		{
			_pRCT->RefShapeDraw()->AABB(_node.box);
		}
	}
	void CollisionWorld::Clear()
	{
		m_staticHandleStorage = {};
		m_staticInstanceIndexVec.clear();
		m_staticInstanceVec.clear();
		m_staticNodeVec.clear();
	}
}