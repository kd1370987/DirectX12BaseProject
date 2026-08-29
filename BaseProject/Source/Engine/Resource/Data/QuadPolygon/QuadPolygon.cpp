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

		// 頂点バッファ作成
		if (!m_vertexBuffer.CreateAndUpload(
			D3D12::D3D12Wrapper::Instance().GetDevice(),
			4,
			_vertices
		))
		{
			assert(0 && "いたポリの頂点バッファ作成失敗");
		}

		// インデックスバッファ作成
		std::vector<UINT> _indices = { 0,1,2,3,1,0 };
		D3D12::IndexBufferDesc _desc = {};
		_desc.count = _indices.size();
		_desc.pData = _indices.data();
		_desc.format = DXGI_FORMAT_R32_UINT;
		if (!m_indexBuffer.Create(D3D12::D3D12Wrapper::Instance().GetDevice(),_desc))
		{
			assert(0 && "いたポリのインデックスバッファ作成失敗");
		}
	}
	void QuadPolygon::Init(uint32_t a_widthVertNum, uint32_t a_heightVertNum)
	{
		ENGINE_ERRLOG((a_widthVertNum >= 2),"ポリゴンを生成するのに、横の頂点数が足りません");
		ENGINE_ERRLOG((a_heightVertNum >= 2),"ポリゴンを生成するのに、縦の頂点数が足りません");

		// 頂点数
		const size_t _vertNum = static_cast<size_t>(a_widthVertNum * a_heightVertNum);

		// 頂点データ配列
		std::vector<SimpleVertex> _vertices = {};
		_vertices.resize(_vertNum);

		for (uint32_t _y = 0; _y < a_heightVertNum; ++_y)
		{
			// UV_v 0 ～ 1
			const float _v = static_cast<float>(_y) / static_cast<float>(a_heightVertNum - 1);

			// 座標_y -1 ～ 1
			const float _posY = _v * 2.0f - 1.0f;

			for (uint32_t _x = 0; _x < a_widthVertNum; ++_x)
			{
				// UV_u 0 ～ 1
				const float _u = static_cast<float>(_x) / static_cast<float>(a_widthVertNum - 1);

				// 座標_x -1 ～ 1
				const float _posX = _u * 2.0f - 1.0f;

				const size_t _index = static_cast<size_t>(_y) * a_widthVertNum + _x;

				// 頂点データ作成
				// vは上下反転。座標の-1(下)がテクスチャの下端(v=1)に当たる
				// (4頂点版の並びに合わせる)
				_vertices[_index] = {
					{_posX, _posY, 0.0f, 1.0f},
					{_u, 1.0f - _v}
				};
			}
		}

		// 頂点バッファ作成
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

		if (!m_vertexBuffer.CreateAndUpload(_pDevice,_vertNum,_vertices.data()))
		{
			ENGINE_ERRLOG(false, "いたポリの頂点バッファ作成失敗");
		}

		// インデックス数
		const uint32_t _quadWidth = a_widthVertNum - 1;
		const uint32_t _quadHeight = a_heightVertNum - 1;

		// インデックスバッファ作成
		std::vector<UINT> _indices;
		_indices.reserve(static_cast<size_t>(_quadWidth) * static_cast<size_t>(_quadHeight) * 6);

		for (uint32_t _y = 0; _y < _quadHeight; ++_y)
		{
			for (uint32_t _x = 0; _x < _quadWidth; ++_x)
			{
				const uint32_t _topLeft = _y * a_widthVertNum + _x;
				const uint32_t _topRight = _topLeft + 1;
				const uint32_t _bottomLeft = (_y + 1) * a_widthVertNum + _x;
				const uint32_t _bottomRight = _bottomLeft + 1;

				// Triangle 1
				_indices.push_back(_topLeft);
				_indices.push_back(_topRight);
				_indices.push_back(_bottomLeft);

				// Triangle 2
				_indices.push_back(_topRight);
				_indices.push_back(_bottomRight);
				_indices.push_back(_bottomLeft);
			}
		}

		D3D12::IndexBufferDesc _desc = {};
		_desc.count = _indices.size();
		_desc.pData = _indices.data();
		_desc.format = DXGI_FORMAT_R32_UINT;
		if (!m_indexBuffer.Create(D3D12::D3D12Wrapper::Instance().GetDevice(), _desc))
		{
			ENGINE_ERRLOG(false, "いたポリのインデックスバッファ作成失敗");
		}

	}
}
