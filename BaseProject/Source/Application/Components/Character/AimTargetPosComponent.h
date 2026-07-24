#pragma once

//==========================================================================================
// AimTargetPosComponent
//
// カメラがフォーカスしている対象(プレイヤーなど)につけるコンポーネント。
// AimTargetSystem がカメラ正面へレイを飛ばし、その着弾点をワールド座標で書き込む。
// 銃はこの座標へ向けて弾を発射する(GunShootSystem)。
//
// 銃側(子エンティティ)にも同じコンポーネントを付けておくと、
// AttachmentDispatchSystem が親から狙点をコピーしてくれる。
//==========================================================================================
struct AimTargetPosComponent
{
	// ---- 設定(保存される) ----
	float maxDistance = 500.0f;		// レイの長さ。ここまで当たらなければ「最大距離の点」を狙点にする
	float startOffset = 1.0f;		// フォーカス点(自機)から何m先をレイの始点にするか。
									// 自機に付いている武器・ブースターを拾わないための余白。
									// カメラからの絶対距離ではないので、カメラ距離を変えても調整不要。

	// ---- 結果(ランタイム。保存しない) ----
	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };								// 狙点(ワールド座標)
	DirectX::XMFLOAT3 dir = { 0.0f,0.0f,1.0f };								// 狙いの向き(=カメラ前方。単位ベクトル)
	Engine::ECS::Entity hitEntity = Engine::ECS::Limits::INVALID_ENTITY;	// 当たった相手
	bool isHit = false;														// 何かに当たったか
	bool isValid = false;													// 一度でも計算されたか(未計算のpos=原点を撃たないため)
};

template<>
struct Engine::ECS::ComponentTraits<AimTargetPosComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AimTargetPosComponent& _comp = Engine::Editor::GetValue<AimTargetPosComponent>(a_pData);
		a_ar.Field("maxDistance", _comp.maxDistance);
		a_ar.Field("startOffset", _comp.startOffset);
	}

	static void Edit(CompEditContext& a_context)
	{
		AimTargetPosComponent& _comp = Engine::Editor::GetValue<AimTargetPosComponent>(a_context.pData);

		ImGui::DragFloat("MaxDistance", &_comp.maxDistance, 1.0f, 0.0f);
		ImGui::DragFloat("StartOffset", &_comp.startOffset, 0.1f, 0.0f);

		// 結果は表示のみ(システムが毎フレーム上書きする)
		ImGui::Separator();
		ImGui::Text("AimPos : %.2f, %.2f, %.2f", _comp.pos.x, _comp.pos.y, _comp.pos.z);
		ImGui::Text("AimDir : %.2f, %.2f, %.2f", _comp.dir.x, _comp.dir.y, _comp.dir.z);
		ImGui::Text("IsHit  : %s", _comp.isHit ? "true" : "false");
		ImGui::Text("IsValid: %s", _comp.isValid ? "true" : "false");
		ImGui::TextDisabled("HitEntity : %llu", static_cast<unsigned long long>(_comp.hitEntity));
	}
};
