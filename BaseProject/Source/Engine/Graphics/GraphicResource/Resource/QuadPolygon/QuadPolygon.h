#pragma once

#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"

struct SimpleVertex
{
	DirectX::XMFLOAT4 pos = {0.0f,0.0f,0.0f,0.0f};
	DirectX::XMFLOAT2 uv = {0.0f,0.0f};
};

class QuadPolygon
{
public:

	void Init();

	const D3D12_VERTEX_BUFFER_VIEW& GetVBView()
	{
		return m_vertexBuffer.View();
	}

	const D3D12_INDEX_BUFFER_VIEW& GetIBView()
	{
		return m_indexBuffer.View();
	}

private:

	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;
};