#pragma once

namespace Engine::D3D12
{
	class DescriptorHeapManager;
}

namespace Engine::Resource
{
	class QuadPolygon
	{
	public:

		QuadPolygon() = default;
		~QuadPolygon() = default;
		NON_COPYABLE_MOVABLE(QuadPolygon);
		// ビューの置き場(借り物)は呼び出し側から渡す。実体は GraphicsEngine の持ち物
		void Init(D3D12::DescriptorHeapManager* a_pHeapManager);
		void Init(D3D12::DescriptorHeapManager* a_pHeapManager,uint32_t a_widthVertNum,uint32_t a_heightVertNum);

		const D3D12_VERTEX_BUFFER_VIEW& GetVBView()
		{
			return m_vertexBuffer.GetView();
		}

		const D3D12_INDEX_BUFFER_VIEW& GetIBView()
		{
			return m_indexBuffer.GetView();
		}

		// インデックス数(4頂点の1枚板なら6、分割板ならその分だけ増える)。
		// 間接描画の引数(IndexCountPerInstance)を GPU 側で書くときにも使う
		UINT GetIndexCount()
		{
			const D3D12_INDEX_BUFFER_VIEW& _ibView = GetIBView();
			const UINT _indexByteSize = (_ibView.Format == DXGI_FORMAT_R16_UINT) ? 2u : 4u;
			return _ibView.SizeInBytes / _indexByteSize;
		}

	private:

		D3D12::DynamicVertexBuffer<SimpleVertex> m_vertexBuffer;
		D3D12::DynamicIndexBuffer m_indexBuffer;
	};
}