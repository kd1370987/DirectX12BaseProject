#include "MeshEdit.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// メッシュの詳細表示
	//-----------------------------------------------------------------------------------------
	void MeshEdit(EditorContext&, Resource::Mesh* a_pMesh)
	{
		if (!a_pMesh) { return; }

		const auto& _metaData = a_pMesh->GetMetaData();

		// ---- 概要 ----
		Engine::Editor::Value("Vertices", "%zu", a_pMesh->GetVertexVec().size());
		Engine::Editor::Value("Subsets", "%zu", _metaData.subsets.size());
		Engine::Editor::Value("IsSkinMesh", "%s", _metaData.isSkinMesh ? "true" : "false");

		// 実体化しているドメインデータ
		Engine::Editor::Value("Domains", "%s%s%s", a_pMesh->HasRasterData() ? "[Raster]" : "", a_pMesh->HasRtData() ? "[Raytracing]" : "", a_pMesh->HasMeshShaderData() ? "[MeshShader]" : "");

		Engine::Editor::Line();

		// ---- 境界ボリューム ----
		if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const auto& _aabb = _metaData.aabb;
			Engine::Editor::Value("AABB Center", "%.3f, %.3f, %.3f", _aabb.Center.x, _aabb.Center.y, _aabb.Center.z);
			Engine::Editor::Value("AABB Extents", "%.3f, %.3f, %.3f", _aabb.Extents.x, _aabb.Extents.y, _aabb.Extents.z);

			const auto& _bSphere = _metaData.bSphere;
			Engine::Editor::Value("Sphere Center", "%.3f, %.3f, %.3f", _bSphere.Center.x, _bSphere.Center.y, _bSphere.Center.z);
			Engine::Editor::Value("Sphere Radius", "%.3f", _bSphere.Radius);
		}

		// ---- サブセット ----
		if (ImGui::CollapsingHeader("Subsets"))
		{
			for (size_t _i = 0; _i < _metaData.subsets.size(); ++_i)
			{
				const auto& _subset = _metaData.subsets[_i];
				Engine::Editor::Text("[%zu] material=%u faceStart=%u faceCount=%u", _i, _subset.materialNumber, _subset.faceStart, _subset.faceCount);
			}
		}

		// ---- レイトレーシングデータ ----
		if (a_pMesh->HasRtData() && ImGui::CollapsingHeader("Raytracing Data"))
		{
			const auto& _rtData = a_pMesh->GetRtData();

			Engine::Editor::Header("VertexHandle");
			HandleInfo(_rtData.vertexHandle);
			Engine::Editor::Header("IndexHandle");
			HandleInfo(_rtData.indexHandle);
		}

		// ---- メッシュシェーダーデータ ----
		if (a_pMesh->HasMeshShaderData() && ImGui::CollapsingHeader("MeshShader Data"))
		{
			const auto& _meshShaderData = a_pMesh->GetMeshShaderData();

			Engine::Editor::Value("Meshlets", "%zu", _meshShaderData.meshlets.size());
			Engine::Editor::Value("UniqueVertexIndices", "%zu", _meshShaderData.uniqueVertexIndices.size());
			Engine::Editor::Value("PrimitiveIndices", "%zu", _meshShaderData.primitiveIndices.size());
			Engine::Editor::Value("CullData", "%zu", _meshShaderData.cullData.size());

			if (ImGui::TreeNode("SubsetMeshlets"))
			{
				for (size_t _i = 0; _i < _meshShaderData.subsetMeshlets.size(); ++_i)
				{
					const auto& _subsetMeshlet = _meshShaderData.subsetMeshlets[_i];
					Engine::Editor::Text("[%zu] meshletOffset=%u meshletCount=%u cullOffset=%u cullCount=%u", _i, _subsetMeshlet.meshletOffset, _subsetMeshlet.meshletCount, _subsetMeshlet.cullOffset, _subsetMeshlet.cullCount);
				}
				ImGui::TreePop();
			}
		}
	}
}
