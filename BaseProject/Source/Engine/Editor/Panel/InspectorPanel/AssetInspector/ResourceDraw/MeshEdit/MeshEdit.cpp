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
		ImGui::Text("Vertices   : %zu", a_pMesh->GetVertexVec().size());
		ImGui::Text("Subsets    : %zu", _metaData.subsets.size());
		ImGui::Text("IsSkinMesh : %s", _metaData.isSkinMesh ? "true" : "false");

		// 実体化しているドメインデータ
		ImGui::Text(
			"Domains    : %s%s%s%s",
			a_pMesh->HasRasterData() ? "[Raster]" : "",
			a_pMesh->HasRtData() ? "[Raytracing]" : "",
			a_pMesh->HasMeshShaderData() ? "[MeshShader]" : "",
			a_pMesh->HasCollisionMesh() ? "[Collision]" : ""
		);

		ImGui::Separator();

		// ---- 境界ボリューム ----
		if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const auto& _aabb = _metaData.aabb;
			ImGui::Text("AABB Center : %.3f, %.3f, %.3f", _aabb.Center.x, _aabb.Center.y, _aabb.Center.z);
			ImGui::Text("AABB Extents: %.3f, %.3f, %.3f", _aabb.Extents.x, _aabb.Extents.y, _aabb.Extents.z);

			const auto& _bSphere = _metaData.bSphere;
			ImGui::Text("Sphere Center: %.3f, %.3f, %.3f", _bSphere.Center.x, _bSphere.Center.y, _bSphere.Center.z);
			ImGui::Text("Sphere Radius: %.3f", _bSphere.Radius);
		}

		// ---- サブセット ----
		if (ImGui::CollapsingHeader("Subsets"))
		{
			for (size_t _i = 0; _i < _metaData.subsets.size(); ++_i)
			{
				const auto& _subset = _metaData.subsets[_i];
				ImGui::Text(
					"[%zu] material=%u faceStart=%u faceCount=%u",
					_i, _subset.materialNumber, _subset.faceStart, _subset.faceCount
				);
			}
		}

		// ---- レイトレーシングデータ ----
		if (a_pMesh->HasRtData() && ImGui::CollapsingHeader("Raytracing Data"))
		{
			const auto& _rtData = a_pMesh->GetRtData();

			ImGui::Text("VertexHandle");
			Helper::DrawHandle(_rtData.vertexHandle);
			ImGui::Separator();
			ImGui::Text("IndexHandle");
			Helper::DrawHandle(_rtData.indexHandle);
		}

		// ---- メッシュシェーダーデータ ----
		if (a_pMesh->HasMeshShaderData() && ImGui::CollapsingHeader("MeshShader Data"))
		{
			const auto& _meshShaderData = a_pMesh->GetMeshShaderData();

			ImGui::Text("Meshlets            : %zu", _meshShaderData.meshlets.size());
			ImGui::Text("UniqueVertexIndices : %zu", _meshShaderData.uniqueVertexIndices.size());
			ImGui::Text("PrimitiveIndices    : %zu", _meshShaderData.primitiveIndices.size());
			ImGui::Text("CullData            : %zu", _meshShaderData.cullData.size());

			if (ImGui::TreeNode("SubsetMeshlets"))
			{
				for (size_t _i = 0; _i < _meshShaderData.subsetMeshlets.size(); ++_i)
				{
					const auto& _subsetMeshlet = _meshShaderData.subsetMeshlets[_i];
					ImGui::Text(
						"[%zu] meshletOffset=%u meshletCount=%u cullOffset=%u cullCount=%u",
						_i,
						_subsetMeshlet.meshletOffset, _subsetMeshlet.meshletCount,
						_subsetMeshlet.cullOffset, _subsetMeshlet.cullCount
					);
				}
				ImGui::TreePop();
			}
		}

		// ---- 当たり判定データ ----
		if (a_pMesh->HasCollisionMesh() && ImGui::CollapsingHeader("Collision Mesh"))
		{
			const auto& _collisionMesh = a_pMesh->GetCollisionMesh();

			ImGui::Text("Triangles     : %zu", _collisionMesh.triangleVec.size());
			ImGui::Text("BVHNodes      : %zu", _collisionMesh.nodeVec.size());
			ImGui::Text("RootNodeIndex : %d", _collisionMesh.rootNodeIndex);

			const auto& _localAABB = _collisionMesh._localAABB;
			ImGui::Text("LocalAABB Center : %.3f, %.3f, %.3f", _localAABB.Center.x, _localAABB.Center.y, _localAABB.Center.z);
			ImGui::Text("LocalAABB Extents: %.3f, %.3f, %.3f", _localAABB.Extents.x, _localAABB.Extents.y, _localAABB.Extents.z);
		}
	}
}
