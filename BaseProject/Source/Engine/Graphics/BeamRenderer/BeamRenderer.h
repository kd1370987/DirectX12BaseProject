#pragma once
namespace Engine::Graphics
{
	// GPUに送るデータ
	struct BeamData
	{
		// 3D空間上での始点と終点
		Math::Vector3 startPos;
		float pad;
		Math::Vector3 endPos;

		// ビーム詳細
		float width;		// 幅
		Math::Color color;	// 色
	};

	class BeamRenderer
	{
	public:

		void Init();

		// ビーム描画
		void DrawBeam(const Math::Vector3& a_startPos, const Math::Vector3& a_endPos, float a_width, const Math::Color& a_color);
		void DrawBeam(const Math::Vector3& a_startPos, float a_length, float a_width, const Math::Color& a_color);

	private:

		D3D12::StaticStructuredBuffer<BeamData> m_beamBuffer;

	};
}