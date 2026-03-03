#include "Pch.h"

bool Collider::RayVsMesh(
	const RayInfo& a_ray, 
	DirectX::XMVECTOR v0,
	DirectX::XMVECTOR v1,
	DirectX::XMVECTOR v2,
	float& a_outT
)
{
	constexpr float EPS = static_cast<float>(0.000001);

	DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(v1,v0);
	DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(v2, v0);

	DirectX::XMVECTOR _dir = DirectX::XMLoadFloat3(&a_ray.direction);
	DirectX::XMVECTOR pvec = DirectX::XMVector3Cross(_dir, edge2);
	float _det = DirectX::XMVectorGetX(DirectX::XMVector3Dot(edge1, pvec));
	if (fabs(_det) < EPS)
	{
		return false;
	}

	float _invDet = 1.0f / _det;
	DirectX::XMVECTOR _origin = DirectX::XMLoadFloat3(&a_ray.origin);
	DirectX::XMVECTOR _tvec = DirectX::XMVectorSubtract(_origin, v0);

	float _u = DirectX::XMVectorGetX(DirectX::XMVector3Dot(_tvec, pvec)) * _invDet;
	if (_u < 0.0f || _u > 1.0f)
	{
		return false;
	}

	DirectX::XMVECTOR _qvec = DirectX::XMVector3Cross(_tvec, edge1);
	float _v = DirectX::XMVectorGetX(DirectX::XMVector3Dot(_dir, _qvec)) * _invDet;
	if (_v < 0.0f || _u + _v > 1.0f)
	{
		return false;
	}

	float _dist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(edge2, _qvec)) * _invDet;
	if(_dist < 0.0f || _dist > a_ray.maxDistance)
	{
		return false;
	}

	a_outT = _dist;
	return true;
}
