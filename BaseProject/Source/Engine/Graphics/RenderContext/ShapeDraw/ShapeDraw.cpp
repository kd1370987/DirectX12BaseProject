#include "ShapeDraw.h"

namespace Engine::Graphics
{
	void ShapeRenderer::Line(
		DirectX::XMFLOAT3 a_vertA,
		DirectX::XMFLOAT3 a_vertB,
		DirectX::XMFLOAT4 a_color
	)
	{
		m_vertexVec.push_back({ a_vertA,a_color });
		m_vertexVec.push_back({ a_vertB,a_color });
	}

	void ShapeRenderer::AABB(
		const DirectX::BoundingBox& a_box,
		DirectX::XMFLOAT4 a_color
	)
	{
		// ８頂点生成
		DirectX::XMFLOAT3 _verts[8];
		auto& _cen = a_box.Center;
		auto& _ext = a_box.Extents;
		// 下面
		_verts[0] = { _cen.x - _ext.x,_cen.y - _ext.y,_cen.z - _ext.z };		// 左下
		_verts[1] = { _cen.x + _ext.x,_cen.y - _ext.y,_cen.z - _ext.z };		// 右下
		_verts[2] = { _cen.x - _ext.x,_cen.y - _ext.y,_cen.z + _ext.z };		// 左上
		_verts[3] = { _cen.x + _ext.x,_cen.y - _ext.y,_cen.z + _ext.z };		// 右上
		// 上面
		_verts[4] = { _cen.x - _ext.x,_cen.y + _ext.y,_cen.z - _ext.z };		// 左下
		_verts[5] = { _cen.x + _ext.x,_cen.y + _ext.y,_cen.z - _ext.z };		// 左下
		_verts[6] = { _cen.x - _ext.x,_cen.y + _ext.y,_cen.z + _ext.z };		// 左下
		_verts[7] = { _cen.x + _ext.x,_cen.y + _ext.y,_cen.z + _ext.z };		// 左下

		// LINELISTのため12本描画
		// 下辺
		Line(_verts[0], _verts[1]);
		Line(_verts[1], _verts[3]);
		Line(_verts[3], _verts[2]);
		Line(_verts[2], _verts[0]);

		// 上辺
		Line(_verts[4], _verts[5]);
		Line(_verts[5], _verts[7]);
		Line(_verts[7], _verts[6]);
		Line(_verts[6], _verts[4]);

		// 縦辺
		Line(_verts[0], _verts[4]);
		Line(_verts[1], _verts[5]);
		Line(_verts[2], _verts[6]);
		Line(_verts[3], _verts[7]);
	}

	void ShapeRenderer::Sphere(
		DirectX::XMFLOAT3 a_center,
		float a_radius,
		DirectX::XMFLOAT4 a_color
	)
	{}

}