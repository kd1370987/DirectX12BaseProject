#include "TestTriangle.h"

#include "../PrimitiveHelper/PrimitiveHelper.h"

namespace Engine::Collision::NarrowPhase
{
	bool Engine::Collision::NarrowPhase::TestTriangle(
		const RayInfo& a_ray, 
		const DirectX::XMVECTOR& a_v0, 
		const DirectX::XMVECTOR& a_v1, 
		const DirectX::XMVECTOR& a_v2, 
		float& a_outDist
	)
	{
		// イプシロン
		// 浮動小数点誤差のための最小値
		constexpr float _EPS = static_cast<float>(1e-6f);

		// 辺を求める
		DirectX::XMVECTOR _edge1 = DirectX::XMVectorSubtract(a_v1, a_v0);	// a
		DirectX::XMVECTOR _edge2 = DirectX::XMVectorSubtract(a_v2, a_v0);	// b

		// レイの発射方向
		DirectX::XMVECTOR _dir = DirectX::XMLoadFloat3(&a_ray.direction);

		// レイと三角形が平行かどうかを求める
		// _det == 0  : 平行
		// _det > 0   : 表面からヒット
		// _det < 0   : 裏面からヒット
		DirectX::XMVECTOR _pvec = DirectX::XMVector3Cross(_dir,_edge2);
		float _det = DirectX::XMVectorGetX(DirectX::XMVector3Dot(_edge1, _pvec));
		if (std::fabs(_det) < _EPS)
		{
			// イプシロン以下はほぼ平行なためヒットすることはないのでリターン
			return false;
		}

		// 除算を一度する
		// 後半での除算回数を減らすため
		float _invDet = 1.0f / _det;

		// レイの発射座標
		DirectX::XMVECTOR _origin = DirectX::XMLoadFloat3(&a_ray.origin);
		DirectX::XMVECTOR _tvec = DirectX::XMVectorSubtract(_origin,a_v0);

		// u座標を求める
		// バリセントリック座標でのu座標 : 三角形の頂点を基準に内部の任意の点を重みづけで表す座標
		// _u < 0.0f : 左に飛び出している
		// _u > 1.0f : 右に飛び出している
		float _u = DirectX::XMVectorGetX(DirectX::XMVector3Dot(_tvec,_pvec)) * _invDet;
		if (_u < 0.0f || _u > 1.0f)
		{
			// uが三角形外に飛び出しているためリターン
			return false;
		}

		// v座標を求める
		// バリセントリック座標でのv座標
		// _v < 0.0f		: 上側に飛び出している
		// _u + _v > 1.0f	: 斜辺を超えている
		DirectX::XMVECTOR _qvec = DirectX::XMVector3Cross(_tvec,_edge1);	// vのための補助ベクトル
		float _v = DirectX::XMVectorGetX(DirectX::XMVector3Dot(_dir, _qvec)) * _invDet;
		if (_v < 0.0f || _u + _v > 1.0f)
		{
			// _vが三角形外に飛び出しているためリターン
			return false;
		}

		// 交点との距離_tを求める
		// _t < 0.0f				: 後ろ方向
		// _t > a_ray.maxDistance	: 距離範囲外
		float _t = DirectX::XMVectorGetX(DirectX::XMVector3Dot(_edge2, _qvec)) * _invDet;
		if (_t < 0.0f || _t > a_ray.maxDistance)
		{
			// _tが範囲外だったためリターン
			return false;
		}

		// ヒット
		// 交点との距離を引数に返す
		a_outDist = _t;
		return true;
	}

	bool TestTriangle(
		const SphereInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist)
	{
		a_outDist = 0.0f;
		return MakeSphere(a_info).Intersects(a_v0, a_v1, a_v2);
	}

	bool TestTriangle(
		const CapsuleInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist)
	{
		a_outDist = 0.0f;

		// 三角形の頂点を取り出す
		DXSM::Vector3 _a, _b, _c;
		DirectX::XMStoreFloat3(&_a, a_v0);
		DirectX::XMStoreFloat3(&_b, a_v1);
		DirectX::XMStoreFloat3(&_c, a_v2);

		// 線分と三角形の最近接距離が半径以下なら当たり
		float _distSq = ClosestDistSqSegmentTriangle(a_info.pointA, a_info.pointB, _a, _b, _c);
		return _distSq <= a_info.radius * a_info.radius;
	}

	bool TestTriangle(
		const OBBInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist)
	{
		a_outDist = 0.0f;
		return MakeOBB(a_info).Intersects(a_v0, a_v1, a_v2);
	}

	bool TestTriangle(
		const FrustumInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist)
	{
		a_outDist = 0.0f;
		return MakeFrustum(a_info).Intersects(a_v0, a_v1, a_v2);
	}
}