#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// ディファードライティングでシェーダーに送る調整値。
	// これまでシェーダー内にハードコードされていた値をエディタから触れるようにする。
	// (実際の送信は DeferredLighting パスが定数バッファに詰めて行う)
	struct LightingOption : IOption
	{
		float giIntensity = 1.0f;			// GI(間接光)の強さ
		float directionalIntensity = 1.0f;	// 平行光(直接光)の強さ
		float dielectricF0 = 0.04f;			// 非金属の基本反射率(スペキュラF0)

		const std::string& GetName() const override
		{
			static const std::string NAME = "LightingOption";
			return NAME;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		void DrawEdit(const ECS::EngineServices&) override
		{
			Engine::EditorField::Field("GI Intensity", giIntensity, 0.01f, 0.0f, 10.0f);
			Engine::EditorField::Field("Directional Intensity", directionalIntensity, 0.01f, 0.0f, 10.0f);
			Engine::EditorField::Field("Dielectric F0", dielectricF0, 0.001f, 0.0f, 1.0f);
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("giIntensity", giIntensity);
			a_archive.Field("directionalIntensity", directionalIntensity);
			a_archive.Field("dielectricF0", dielectricF0);
		}
	};
}
