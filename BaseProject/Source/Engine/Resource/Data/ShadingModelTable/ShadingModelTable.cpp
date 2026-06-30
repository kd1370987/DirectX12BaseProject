#include "ShadingModelTable.h"

#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	std::span<const Handle<Shader>> Engine::Resource::ShadingModelTable::GetShaderHandles(UINT a_passHash) const
	{
		auto _it = m_shaderHandleMap.find(a_passHash);
		if (_it != m_shaderHandleMap.end())
		{
			return _it->second;
		}
		return {};
	}
	std::span<const Handle<ShaderLibrary>> ShadingModelTable::GetShaderLibraryHandles(UINT a_passHash) const
	{
		auto _it = m_shaderLibaryHandleMap.find(a_passHash);
		if (_it != m_shaderLibaryHandleMap.end())
		{
			return _it->second;
		}
		return {};
	}
	void ShadingModelTable::Archive(Persistence::Archive& a_ar)
	{

	}
	void ShadingModelTable::Edit()
	{
		ImGui::Text("Shading Model: %s", m_typeName.c_str());
		ImGui::Separator();

		// 保存ボタン（パス取得やセーブ処理は仮置き）
		if (ImGui::Button("Save Asset"))
		{
			// TODO: セーブパスを取得する
			// std::string savePath = GetSavePath();
			// Persistence::OutputArchive ar(savePath);
			// Archive(ar);
		}

		ImGui::Spacing();

		// リソースマネージャーから配列を取得
		auto _shaderVec = AssetDatabase::Instance().GetTypeMetaVec("Shader");

		// 通常シェーダーとライブラリでタブを分ける
		if (ImGui::BeginTabBar("ShadingModelTabs"))
		{
			if (ImGui::BeginTabItem("Raster Shaders"))
			{
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Shader Libraries (DXR)"))
			{
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
}