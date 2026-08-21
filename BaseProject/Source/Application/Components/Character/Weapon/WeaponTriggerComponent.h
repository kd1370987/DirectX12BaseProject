#pragma once

//==========================================================================================
// WeaponTriggerComponent
//
// 武器エンティティが外から受け取る「命令」だけを持つコンポーネント。
//
// 持ち主(プレイヤー・敵)は「左を撃て / 右を撃て」としか言わない。
// 撃てるかどうか(連射間隔・バースト・熱)や、何をどう撃つか(弾・弾速・銃口)は
// 武器側の GunStateComponent が持ち、GunShootSystem が面倒を見る。
//
// 引き金を引くのは
//   ・持ち主が武器を子として持つ場合 : AttachmentDispatchSystem
//   ・持ち主自身が武器を兼ねる場合   : SelfWeaponTriggerSystem
// のどちらか。武器側から持ち主を見に行くことはしない。
//
// 保存するものは無い(毎フレーム上書きされる命令なので)。
//==========================================================================================
struct WeaponTriggerComponent
{
	// 引き金を引かれているか。押されている間ずっと true
	bool isPulled = false;
};

template<>
struct Engine::ECS::ComponentTraits<WeaponTriggerComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		// ランタイム専用。保存するフィールドは無い
	}

	static void Edit(CompEditContext& a_context)
	{
		WeaponTriggerComponent& _comp = Engine::Editor::GetValue<WeaponTriggerComponent>(a_context.pData);

		// 配信された結果を見るだけ。ここから触っても次のフレームで上書きされる
		bool _pulled = _comp.isPulled;
		ImGui::BeginDisabled(true);
		ImGui::Checkbox("isPulled", &_pulled);
		ImGui::EndDisabled();
	}
};
