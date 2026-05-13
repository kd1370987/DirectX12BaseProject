#pragma once
namespace Engine::Graphics
{


	class ShapeRenderer
	{
	public:

		// 線描画
		void Line(
			DirectX::XMFLOAT3 a_vertA,
			DirectX::XMFLOAT3 a_vertB,
			DirectX::XMFLOAT4 a_color = Engine::Color::RED
		);
		// 箱描画
		void AABB(
			const DirectX::BoundingBox& a_box,
			DirectX::XMFLOAT4 a_color = Engine::Color::RED
		);
		// 球描画
		void Sphere(
			DirectX::XMFLOAT3 a_center,
			float a_radius,
			DirectX::XMFLOAT4 a_color = Engine::Color::RED
		);
		//void Capsule();

		const std::vector<Resource::Vertex>& GetVertexVec()
		{
			return m_vertexVec;
		}

		const UINT GetMaxCount()
		{
			return m_maxVertexCount;
		}

		void Reset()
		{
			m_vertexVec.clear();
			m_vertexVec.reserve(m_maxVertexCount);
		}

	private:
		const UINT m_maxVertexCount = 10000000;

		std::vector<Resource::Vertex> m_vertexVec = {};
	};
}

