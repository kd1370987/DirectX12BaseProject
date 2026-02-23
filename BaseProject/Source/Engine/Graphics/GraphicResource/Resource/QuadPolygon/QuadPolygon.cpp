#include "QuadPolygon.h"

void QuadPolygon::Init()
{
	SimpleVertex _vertices[] = {
		{
			{-1.0f,-1.0f,0.0f,1.0f},
			{0.0f,1.0f}
		},
		{
			{1.0f,1.0f,0.0f,1.0f},
			{1.0f,0.0f}
		},
		{
			{1.0f,-1.0f,0.0f,1.0f},
			{1.0f,1.0f}
		},
		{
			{-1.0f,1.0f,0.0f,1.0f},
			{0.0f,0.0f}
		}
	};

	if (!m_vertexBuffer.Create(
		4,
		sizeof(SimpleVertex),
		_vertices
	))
	{
		assert(0 && "いたポリの頂点バッファ作成失敗");
	}

	std::vector<UINT> _indices = { 0,1,2,3,1,0 };
	if (!m_indexBuffer.Create(
		(UINT)_indices.size(),
		sizeof(UINT),
		_indices.data()
	))
	{
		assert(0 && "いたポリのインデックスバッファ作成失敗");
	}
}
