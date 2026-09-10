#pragma once

namespace Engine::Resource
{

	// 頂点
	struct Vertex
	{
		Math::Vector3 pos = {};
		Math::Color color = Engine::Color::RED;
	};

	struct SimpleVertex
	{
		Math::Vector4 pos = { 0.0f,0.0f,0.0f,0.0f };
		Math::Vector2 uv = { 0.0f,0.0f };
	};

	//==========================================================
	// メッシュ用 頂点情報
	//==========================================================
	struct MeshVertex8bit
	{
		Math::Vector3		pos = { 0.0f,0.0f,0.0f };				// 座標
		Math::Vector2		uv = { 0.0f,0.0f };					// uv座標
		unsigned int			color = 0xFFFFFFFF;					// RGBA(各色0～255のUINT型)
		Math::Vector3		normal = { 0.5f,0.5f,0.5f };			// 法線
		Math::Vector3		tangent = { 1.0f,1.0f,1.0f };				// 接線

		std::array<short, 4>	skinIndexList = {};			// スキニングIndexリスト
		std::array<float, 4>	skinWeightList = {};			// スキニングウェイトリスト
	};
	struct MeshVertexFloat
	{
		Math::Vector3		pos = { 0.0f,0.0f,0.0f };					// 座標
		Math::Vector3		normal = { 0.5f,0.5f,0.5f };					// 法線
		Math::Vector2		uv = { 0.0f,0.0f };						// uv座標
		Math::Vector3		tangent = { 1.0f,1.0f,1.0f };				// 接線
		Math::Color			color = { 1.0f,1.0f,1.0f,1.0f };					// RGBA(各色0.0f～1.0fのFLOAT型)

		std::array<short, 4>	skinIndexList = {};			// スキニングIndexリスト
		std::array<float, 4>	skinWeightList = {};			// スキニングウェイトリスト
	};

	//==========================================================
	// レイトレ
	//==========================================================
	struct RTVertex
	{
		Math::Vector3 pos;
		Math::Vector3 normal;
		Math::Vector2 uv;
		Math::Vector3 tangent;
		Math::Color color;

		RTVertex& operator=(const MeshVertexFloat& a_vertFloat)
		{
			pos = a_vertFloat.pos;
			normal = a_vertFloat.normal;
			uv = a_vertFloat.uv;
			tangent = a_vertFloat.tangent;
			color = a_vertFloat.color;

			return *this;
		}
	};
}