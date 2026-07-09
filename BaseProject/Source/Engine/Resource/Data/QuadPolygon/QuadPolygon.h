#pragma once
namespace Engine::Resource
{
	class QuadPolygon
	{
	public:

		QuadPolygon() = default;
		~QuadPolygon() = default;
		NON_COPYABLE_MOVABLE(QuadPolygon);
		void Init();

		const D3D12_VERTEX_BUFFER_VIEW& GetVBView()
		{
			return m_vertexBuffer.GetView();
		}

		const D3D12_INDEX_BUFFER_VIEW& GetIBView()
		{
			return m_indexBuffer.GetView();
		}

	private:

		D3D12::DynamicVertexBuffer<SimpleVertex> m_vertexBuffer;
		D3D12::DynamicIndexBuffer m_indexBuffer;
	};
}