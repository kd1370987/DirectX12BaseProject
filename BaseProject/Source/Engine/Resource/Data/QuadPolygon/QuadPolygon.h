#pragma once
namespace Engine::Resource
{
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
}