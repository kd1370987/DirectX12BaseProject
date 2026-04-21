#pragma once

struct ProjMatComponent
{
	DirectX::XMFLOAT4X4 projMat = {};     // 射影行列
	DirectX::XMFLOAT4X4 projInvMat = {};  // 射影逆行列

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const ProjMatComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<ProjMatComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		ProjMatComponent& _comp = Engine::Editor::GetValue<ProjMatComponent>(a_data);
		{
			ImGui::Text("ProjctionMat");
			float* m = (float*)_comp.projMat.m;
			for (int i = 0; i < 4; ++i)
			{
				ImGui::DragFloat4("##row", &m[i * 4]);
			}
		}

		{
			ImGui::Text("ProjctionInverseMat");
			float* m = (float*)_comp.projInvMat.m;
			for (int i = 0; i < 4; ++i)
			{
				ImGui::DragFloat4("##row", &m[i * 4]);
			}
		}
		
	}
};