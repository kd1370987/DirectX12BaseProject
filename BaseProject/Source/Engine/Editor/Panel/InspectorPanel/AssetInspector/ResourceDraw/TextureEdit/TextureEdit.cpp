#include "TextureEdit.h"

#include "../../../../../../Option/OptionManager.h"
#include "../../../../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		//-----------------------------------------------------------------------------------------
		// 使用フラグを文字列化
		//-----------------------------------------------------------------------------------------
		std::string MakeUsageString(Resource::TextureUsage a_usage)
		{
			if (a_usage == Resource::TextureUsage::None) { return "None"; }

			std::string _usageStr;
			for (auto _flag : magic_enum::enum_values<Resource::TextureUsage>())
			{
				if (_flag == Resource::TextureUsage::None) { continue; }

				// 立っているフラグのみ連結
				bool _hasFlag = (static_cast<uint32_t>(a_usage) & static_cast<uint32_t>(_flag)) != 0;
				if (!_hasFlag) { continue; }

				if (!_usageStr.empty()) { _usageStr += " | "; }
				_usageStr += magic_enum::enum_name(_flag);
			}
			return _usageStr;
		}
	}

	//-----------------------------------------------------------------------------------------
	// テクスチャの詳細表示
	//-----------------------------------------------------------------------------------------
	void TextureEdit(EditorContext& a_editContext, Resource::Texture* a_pTexture)
	{
		if (!a_pTexture) { return; }

		auto _guid = a_editContext.pAssetProp->guid;

		// 画像の保存
		if (ImGui::Button("Save to DDS"))
		{
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			a_pTexture->Save(_filePath);
			ENGINE_LOG("テクスチャの保存が完了 : %s", _filePath.c_str());
		}

		ImGui::Separator();

		// ---- リソース情報 ----
		const auto& _desc = a_pTexture->GetDesc();
		ImGui::Text("Name       : %s", a_pTexture->GetName().c_str());
		ImGui::Text("Size       : %llu x %u", _desc.Width, _desc.Height);
		ImGui::Text("MipLevels  : %u", static_cast<UINT>(_desc.MipLevels));
		ImGui::Text("ArraySize  : %u", static_cast<UINT>(_desc.DepthOrArraySize));
		ImGui::Text("Format     : %s", std::string(magic_enum::enum_name(_desc.Format)).c_str());
		ImGui::Text("SampleCount: %u", _desc.SampleDesc.Count);
		ImGui::Text("Usage      : %s", MakeUsageString(a_pTexture->GetUsage()).c_str());

		const auto& _clearColor = a_pTexture->GetClearColor();
		ImGui::Text("ClearColor : %.3f, %.3f, %.3f, %.3f", _clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);

		ImGui::Separator();

		// ---- 画像の描画 ----
		auto _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(a_pTexture->GetImGuiSRV());
		Helper::DrawSRVView(_gpuHandle, _winOp.windowWidth, _winOp.windowHegiht);
	}
}
