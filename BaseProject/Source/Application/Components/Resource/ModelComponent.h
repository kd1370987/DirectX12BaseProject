#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../../../Engine/Editor/EditorUI/EditorUI.h"

#include "../../../Engine/ECS/World/World.h"

struct ModelComponent
{
	DirectX::XMFLOAT4 colorScale = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 emissiveScale = { 1.0f,1.0f,1.0f };

	// モデル参照用
	Engine::Handle<Engine::Resource::Model> handle = {};	// ランタイム用
	Engine::GUID modelGUID = {};									// 記録用
};

template<>
struct Engine::ECS::ComponentTraits<ModelComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ModelComponent& _comp = Engine::Editor::GetValue<ModelComponent>(a_pData);
		a_ar.Field("colorScale", _comp.colorScale);
		a_ar.Field("emissiveScale", _comp.emissiveScale);
		a_ar.Field("modelGUID", _comp.modelGUID);
	}

	static void Edit(CompEditContext& a_context)
	{

		ModelComponent& _comp = Engine::Editor::GetValue<ModelComponent>(a_context.pData);

		// ---------------------------------------------------------
		// モデルの選択UI
		// ---------------------------------------------------------
		// ここではGUIDのみを書き換え、handleは旧モデルのまま残す。
		// 即時にhandleを差し替えると、今フレームの描画が
		// 「新モデルの描画コマンド + 旧モデルサイズのノードポーズ領域」で走り、spanが範囲外になる。
		// 差し替えはリフレッシュ経路に任せる :
		// Release(旧handleで領域解放) → ModelFixupSystemがGUIDから新handleを復元 → 新サイズで領域再確保
		if (Engine::Editor::UI::DrawAssetSelectComboGUID(
			"Change Model",
			"Model",
			_comp.modelGUID))
		{
			// 実体を持つエンティティのときだけリフレッシュ経路に乗せる。
			// プレハブ編集では実体が無く entity は INVALID なので、
			// GUID の書き換えだけ行い、リフレッシュはしない(無効IDで参照するとレンジ外になる)。
			if (a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
			{
				a_context.pWorld->AddRefreshEntity(a_context.entity);
			}
		}

		ImGui::Text("EmissiveScale");
		ImGui::ColorPicker4("EmissiveScale", (float*)&_comp.emissiveScale.x);

		ImGui::Text("ColorScale");
		ImGui::ColorPicker4("ColorScale", (float*)&_comp.colorScale.x);
	}
};