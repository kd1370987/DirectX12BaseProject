#pragma once

// =====================================================================================
// プリミティブ情報(Info構造体) から DirectX の境界ボリュームを作るヘルパーと、
// DirectX に無いカプセル(線分＋半径)用の幾何計算をまとめたヘッダ。
// NarrowPhase の各判定から共通で使う。
// =====================================================================================
namespace Engine::Collision::NarrowPhase
{
	// 球
	inline DirectX::BoundingSphere MakeSphere(const SphereInfo& a_info)
	{
		DirectX::BoundingSphere _s;
		_s.Center = a_info.origin;
		_s.Radius = a_info.radius;
		return _s;
	}

	// OBB
	inline DirectX::BoundingOrientedBox MakeOBB(const OBBInfo& a_info)
	{
		DirectX::BoundingOrientedBox _b;
		_b.Center = a_info.center;
		_b.Extents = a_info.extents;
		_b.Orientation = a_info.orientation;
		return _b;
	}

	// フラスタム
	inline DirectX::BoundingFrustum MakeFrustum(const FrustumInfo& a_info)
	{
		DirectX::BoundingFrustum _f;
		_f.Origin = a_info.origin;
		_f.Orientation = a_info.orientation;
		_f.RightSlope = a_info.rightSlope;
		_f.LeftSlope = a_info.leftSlope;
		_f.TopSlope = a_info.topSlope;
		_f.BottomSlope = a_info.bottomSlope;
		_f.Near = a_info.nearPlane;
		_f.Far = a_info.farPlane;
		return _f;
	}

	// カプセルを包む保守的な AABB（ブロードフェーズ用）
	inline DirectX::BoundingBox MakeCapsuleAABB(const CapsuleInfo& a_info)
	{
		DXSM::Vector3 _min = DXSM::Vector3::Min(a_info.pointA, a_info.pointB);
		DXSM::Vector3 _max = DXSM::Vector3::Max(a_info.pointA, a_info.pointB);
		DXSM::Vector3 _r = { a_info.radius, a_info.radius, a_info.radius };
		_min -= _r;
		_max += _r;

		DirectX::BoundingBox _box;
		DirectX::BoundingBox::CreateFromPoints(_box, _min, _max);
		return _box;
	}

	// ---- カプセル用の幾何計算（線分と三角形の最近接距離） ------------------------------

	// 点 a_p から三角形 abc への最近接点を返す（Ericson: Real-Time Collision Detection）
	inline DXSM::Vector3 ClosestPtPointTriangle(
		const DXSM::Vector3& a_p,
		const DXSM::Vector3& a_a,
		const DXSM::Vector3& a_b,
		const DXSM::Vector3& a_c)
	{
		DXSM::Vector3 _ab = a_b - a_a;
		DXSM::Vector3 _ac = a_c - a_a;
		DXSM::Vector3 _ap = a_p - a_a;

		float _d1 = _ab.Dot(_ap);
		float _d2 = _ac.Dot(_ap);
		if (_d1 <= 0.0f && _d2 <= 0.0f) return a_a;			// 頂点A領域

		DXSM::Vector3 _bp = a_p - a_b;
		float _d3 = _ab.Dot(_bp);
		float _d4 = _ac.Dot(_bp);
		if (_d3 >= 0.0f && _d4 <= _d3) return a_b;			// 頂点B領域

		float _vc = _d1 * _d4 - _d3 * _d2;
		if (_vc <= 0.0f && _d1 >= 0.0f && _d3 <= 0.0f)		// 辺AB領域
		{
			float _v = _d1 / (_d1 - _d3);
			return a_a + _ab * _v;
		}

		DXSM::Vector3 _cp = a_p - a_c;
		float _d5 = _ab.Dot(_cp);
		float _d6 = _ac.Dot(_cp);
		if (_d6 >= 0.0f && _d5 <= _d6) return a_c;			// 頂点C領域

		float _vb = _d5 * _d2 - _d1 * _d6;
		if (_vb <= 0.0f && _d2 >= 0.0f && _d6 <= 0.0f)		// 辺AC領域
		{
			float _w = _d2 / (_d2 - _d6);
			return a_a + _ac * _w;
		}

		float _va = _d3 * _d6 - _d5 * _d4;
		if (_va <= 0.0f && (_d4 - _d3) >= 0.0f && (_d5 - _d6) >= 0.0f)	// 辺BC領域
		{
			float _w = (_d4 - _d3) / ((_d4 - _d3) + (_d5 - _d6));
			return a_b + (a_c - a_b) * _w;
		}

		// 三角形内部
		float _denom = 1.0f / (_va + _vb + _vc);
		float _v = _vb * _denom;
		float _w = _vc * _denom;
		return a_a + _ab * _v + _ac * _w;
	}

	// 線分 p1q1 と 線分 p2q2 の最近接距離の二乗を返す
	inline float ClosestDistSqSegmentSegment(
		const DXSM::Vector3& a_p1, const DXSM::Vector3& a_q1,
		const DXSM::Vector3& a_p2, const DXSM::Vector3& a_q2)
	{
		constexpr float _EPS = 1e-8f;
		DXSM::Vector3 _d1 = a_q1 - a_p1;	// 線分1の方向
		DXSM::Vector3 _d2 = a_q2 - a_p2;	// 線分2の方向
		DXSM::Vector3 _r = a_p1 - a_p2;
		float _a = _d1.Dot(_d1);
		float _e = _d2.Dot(_d2);
		float _f = _d2.Dot(_r);

		float _s = 0.0f, _t = 0.0f;

		if (_a <= _EPS && _e <= _EPS)
		{
			// 両方が点
			DXSM::Vector3 _diff = a_p1 - a_p2;
			return _diff.Dot(_diff);
		}
		if (_a <= _EPS)
		{
			// 線分1が点
			_t = std::clamp(_f / _e, 0.0f, 1.0f);
		}
		else
		{
			float _c = _d1.Dot(_r);
			if (_e <= _EPS)
			{
				// 線分2が点
				_s = std::clamp(-_c / _a, 0.0f, 1.0f);
			}
			else
			{
				float _b = _d1.Dot(_d2);
				float _denom = _a * _e - _b * _b;
				if (_denom > _EPS) _s = std::clamp((_b * _f - _c * _e) / _denom, 0.0f, 1.0f);
				_t = (_b * _s + _f) / _e;

				if (_t < 0.0f) { _t = 0.0f; _s = std::clamp(-_c / _a, 0.0f, 1.0f); }
				else if (_t > 1.0f) { _t = 1.0f; _s = std::clamp((_b - _c) / _a, 0.0f, 1.0f); }
			}
		}

		DXSM::Vector3 _c1 = a_p1 + _d1 * _s;
		DXSM::Vector3 _c2 = a_p2 + _d2 * _t;
		DXSM::Vector3 _diff = _c1 - _c2;
		return _diff.Dot(_diff);
	}

	// 線分 pq が 三角形 abc を貫いているか（Möller–Trumbore を線分区間で判定）
	inline bool SegmentIntersectsTriangle(
		const DXSM::Vector3& a_p, const DXSM::Vector3& a_q,
		const DXSM::Vector3& a_a, const DXSM::Vector3& a_b, const DXSM::Vector3& a_c)
	{
		constexpr float _EPS = 1e-7f;
		DXSM::Vector3 _dir = a_q - a_p;
		DXSM::Vector3 _e1 = a_b - a_a;
		DXSM::Vector3 _e2 = a_c - a_a;

		DXSM::Vector3 _pvec = _dir.Cross(_e2);
		float _det = _e1.Dot(_pvec);
		if (std::fabs(_det) < _EPS) return false;	// 平行

		float _inv = 1.0f / _det;
		DXSM::Vector3 _tvec = a_p - a_a;
		float _u = _tvec.Dot(_pvec) * _inv;
		if (_u < 0.0f || _u > 1.0f) return false;

		DXSM::Vector3 _qvec = _tvec.Cross(_e1);
		float _v = _dir.Dot(_qvec) * _inv;
		if (_v < 0.0f || _u + _v > 1.0f) return false;

		float _t = _e2.Dot(_qvec) * _inv;
		return (_t >= 0.0f && _t <= 1.0f);	// 線分の区間内で交差
	}

	// 線分 pq と 三角形 abc の最近接距離の二乗を返す
	inline float ClosestDistSqSegmentTriangle(
		const DXSM::Vector3& a_p, const DXSM::Vector3& a_q,
		const DXSM::Vector3& a_a, const DXSM::Vector3& a_b, const DXSM::Vector3& a_c)
	{
		// 面を貫いていれば距離0
		if (SegmentIntersectsTriangle(a_p, a_q, a_a, a_b, a_c)) return 0.0f;

		// 端点 → 三角形
		DXSM::Vector3 _cp = ClosestPtPointTriangle(a_p, a_a, a_b, a_c);
		DXSM::Vector3 _cq = ClosestPtPointTriangle(a_q, a_a, a_b, a_c);
		float _best = std::min((a_p - _cp).LengthSquared(), (a_q - _cq).LengthSquared());

		// 線分 → 三角形の各辺
		_best = std::min(_best, ClosestDistSqSegmentSegment(a_p, a_q, a_a, a_b));
		_best = std::min(_best, ClosestDistSqSegmentSegment(a_p, a_q, a_b, a_c));
		_best = std::min(_best, ClosestDistSqSegmentSegment(a_p, a_q, a_c, a_a));

		return _best;
	}
}
