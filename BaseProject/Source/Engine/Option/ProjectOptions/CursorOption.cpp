#include "CursorOption.h"

#include "../../Editor/Helper/EditorHelper.h"

namespace
{
	// 大きすぎ・小さすぎで見失わないよう幅を決めておく
	constexpr float MIN_SIZE = 8.0f;
	constexpr float MAX_SIZE = 512.0f;
}

void Engine::Option::ProjectOptions::CursorOption::DrawEdit()
{
	ImGui::Checkbox("Enable", &isEnable);
	ImGui::SameLine();
	ImGui::TextDisabled("(切るとOSのカーソルがそのまま出る)");

	ImGui::SeparatorText("Texture");

	Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Cursor", "Texture", textureGUID);
	if (!textureGUID.IsValid())
	{
		ImGui::TextDisabled("(未設定 : OSのカーソルを消さずにそのまま出す)");
	}

	ImGui::SeparatorText("Shape");

	ImGui::DragFloat("Size", &sizePixel, 1.0f, MIN_SIZE, MAX_SIZE, "%.0f px");
	sizePixel = std::clamp(sizePixel, MIN_SIZE, MAX_SIZE);
	ImGui::SameLine();
	ImGui::TextDisabled("(描画解像度基準)");

	// ホットスポットは「画像のどこがカーソルの先端か」。
	// 矢印の絵は余白の中に描かれていることが多く、中心(0.5,0.5)ではまず合わない
	ImGui::DragFloat2("Hotspot", &hotspot.x, 0.005f, 0.0f, 1.0f);
	hotspot.x = std::clamp(hotspot.x, 0.0f, 1.0f);
	hotspot.y = std::clamp(hotspot.y, 0.0f, 1.0f);
	ImGui::TextDisabled("画像の中で実際に指している点(正規化)。矢印なら尖端");

	ImGui::ColorEdit4("Color", color.Data());
}

void Engine::Option::ProjectOptions::CursorOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("isEnable", isEnable);
	a_archive.GUIDField("textureGUID", textureGUID);
	a_archive.Field("sizePixel", sizePixel);
	a_archive.Field("hotspot", hotspot);
	a_archive.Field("color", color);
}
