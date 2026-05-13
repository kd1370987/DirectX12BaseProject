#include "QuadPolygon.h"

#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Resource
{
	void QuadPolygon::Init()
	{
		Engine::Resource::SimpleVertex _vertices[] = {
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

		//if (!m_vertexBuffer.Create(
		//	4,
		//	sizeof(Engine::Resource::SimpleVertex),
		//	_vertices
		//))
		if (!m_vertexBuffer.CreateAndUpload(
			D3D12::D3D12Wrapper::Instance().GetDevice(),
			4,
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
}
