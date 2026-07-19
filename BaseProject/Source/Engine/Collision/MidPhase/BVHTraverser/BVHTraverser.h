#pragma once

#include "../../NarrowPhase/TestAABB/TestAABB.h"
#include "../../NarrowPhase/TestTriangle/TestTriangle.h"

namespace Engine::Collision
{
	// トラバーサークラス
	// BVHの走査のみを担当
	class BVHTraverser
	{
	public:

		// プリミティブ vs メッシュ（レイのような貫通・距離付き判定）
		template<typename TPrimitive>
		static bool Traverse(
			const TPrimitive& a_localPrimitive,					// 判定したいプリミティブ
			const Resource::CollisionMesh& a_collisionMesh,		// 判定するメッシュ
			Result& a_outLocalResult							// 返ってくる結果
		);

		// プリミティブ vs メッシュ（オーバーラップ判定 : 触れているかどうかだけ）
		// 距離での枝刈りが無いため、最初に触れた三角形で早期リターンする
		template<typename TPrimitive>
		static bool TraverseOverlap(
			const TPrimitive& a_localPrimitive,					// 判定したいプリミティブ（メッシュローカル空間）
			const Resource::CollisionMesh& a_collisionMesh,		// 判定するメッシュ
			Result& a_outLocalResult							// 返ってくる結果
		);

	};
	template<typename TPrimitive>
	inline bool BVHTraverser::Traverse(
		const TPrimitive& a_localPrimitive,
		const Resource::CollisionMesh& a_collisionMesh, 
		Result& a_outLocalResult
	)
	{
		// 固定長配列による簡易スタック(再帰呼び出しのオーバーヘッドとメモリ確保を防ぐ)
		int _nodeStack[64];
		int _stackTop = 0;


		// ルートノードをスタックに積む
		_nodeStack[_stackTop++] = a_collisionMesh.rootNodeIndex;

		bool _isHit = false;
		float _closestDist = 999999.0f;			// レイの場合は_maxDistanceを入れる

		while (_stackTop > 0)
		{
			int _currentNodeIdx = _nodeStack[--_stackTop];
			const auto& _node = a_collisionMesh.nodeVec[_currentNodeIdx];
			float _boxDist = 0.0f;

			// NarrowPhaseを呼び出し、AABB判定をする
			if (NarrowPhase::TestAABB(a_localPrimitive, _node.box, _boxDist))
			{

				Editor::MainEditor::Instance().DrawBox(_node.box);

				// 範囲外
				if (_boxDist > _closestDist) continue;

				if (_node.IsLeaf())
				{
					// NarrowPhaseを呼び出し、三角形判定
					for (int _i = 0; _i < _node.dataCount; ++_i)
					{
						// 元のポリゴンインデックスを取得
						int _triIdx = a_collisionMesh.triangleIndiccesVec[_node.dataStart + _i];
						const auto& _triangle = a_collisionMesh.triangleVec[_triIdx];
						// 三角形の頂点取得
						DirectX::XMVECTOR _v0 = DirectX::XMLoadFloat3(&_triangle.v[0]);
						DirectX::XMVECTOR _v1 = DirectX::XMLoadFloat3(&_triangle.v[1]);
						DirectX::XMVECTOR _v2 = DirectX::XMLoadFloat3(&_triangle.v[2]);
						float _triDist = 0.0f;

						// 判定
						if (NarrowPhase::TestTriangle(a_localPrimitive, _v0, _v1, _v2, _triDist))
						{
							// 今回当たったポリゴンが、今までで一番手前かどうかチェック
							if (_triDist < _closestDist)
							{
								// 最接近距離を更新
								_closestDist = _triDist;
								_isHit = true;

								// 結果を代入
								// プリミティブごとに結果を変更
								a_outLocalResult.isHit = true;
								a_outLocalResult.hitDistance = _triDist;
								a_outLocalResult.hitPos.x = a_localPrimitive.origin.x + a_localPrimitive.direction.x * _triDist;
								a_outLocalResult.hitPos.y = a_localPrimitive.origin.y + a_localPrimitive.direction.y * _triDist;
								a_outLocalResult.hitPos.z = a_localPrimitive.origin.z + a_localPrimitive.direction.z * _triDist;
								a_outLocalResult.hitNormal = {};
							}
						}
					}
				}
				else
				{
					// 子ノードをスタックへ戻す
					if (_stackTop < 62)
					{
						_nodeStack[_stackTop++] = _node.leftChild;
						_nodeStack[_stackTop++] = _node.rightChild;
					}
				}
			}
		}
		return _isHit;
	}

	template<typename TPrimitive>
	inline bool BVHTraverser::TraverseOverlap(
		const TPrimitive& a_localPrimitive,
		const Resource::CollisionMesh& a_collisionMesh,
		Result& a_outLocalResult)
	{
		// 固定長配列による簡易スタック
		int _nodeStack[64];
		int _stackTop = 0;

		// ルートノードをスタックに積む
		_nodeStack[_stackTop++] = a_collisionMesh.rootNodeIndex;

		while (_stackTop > 0)
		{
			int _currentNodeIdx = _nodeStack[--_stackTop];
			const auto& _node = a_collisionMesh.nodeVec[_currentNodeIdx];
			float _dummyDist = 0.0f;

			// NarrowPhaseでプリミティブ vs ノードAABB
			if (!NarrowPhase::TestAABB(a_localPrimitive, _node.box, _dummyDist)) continue;

			if (_node.IsLeaf())
			{
				// 葉ノードの三角形と判定
				for (int _i = 0; _i < _node.dataCount; ++_i)
				{
					int _triIdx = a_collisionMesh.triangleIndiccesVec[_node.dataStart + _i];
					const auto& _triangle = a_collisionMesh.triangleVec[_triIdx];

					DirectX::XMVECTOR _v0 = DirectX::XMLoadFloat3(&_triangle.v[0]);
					DirectX::XMVECTOR _v1 = DirectX::XMLoadFloat3(&_triangle.v[1]);
					DirectX::XMVECTOR _v2 = DirectX::XMLoadFloat3(&_triangle.v[2]);

					float _dummyTriDist = 0.0f;
					if (NarrowPhase::TestTriangle(a_localPrimitive, _v0, _v1, _v2, _dummyTriDist))
					{
						// 触れていれば十分なので最初のヒットで返す
						a_outLocalResult.isHit = true;
						a_outLocalResult.hitDistance = 0.0f;

						// 接触点の目安として当たった三角形の重心を返す
						DirectX::XMVECTOR _center = DirectX::XMVectorScale(
							DirectX::XMVectorAdd(DirectX::XMVectorAdd(_v0, _v1), _v2), 1.0f / 3.0f);
						DirectX::XMStoreFloat3(&a_outLocalResult.hitPos, _center);
						return true;
					}
				}
			}
			else
			{
				// 子ノードをスタックへ戻す
				if (_stackTop < 62)
				{
					_nodeStack[_stackTop++] = _node.leftChild;
					_nodeStack[_stackTop++] = _node.rightChild;
				}
			}
		}
		return false;
	}
}