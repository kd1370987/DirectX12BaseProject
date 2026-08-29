#pragma once

//==========================================================================================
// FishEyeComponent
//
// カメラに付ける、魚眼レンズの設定。
//
// center を中心に、外へ向かうほど強く絵を引き伸ばす画面効果。
// ずらす量は中心からの距離の二乗に比例するので、中心付近はほとんど動かず、
// 画面の隅だけが大きく膨らむ = レンズを通したような歪み方になる。
//
//   strength > 0 … 樽型。外側が縮んで画面の四隅に黒が出る(広角レンズ)
//   strength < 0 … 糸巻き型。外側が伸びて絵が画面外へはみ出す
//
// ラジアルブラー(RadialBlurComponent)と同じで、値の置き場所はカメラ。
// CamSetShaderSystem がアクティブカメラのぶんを GraphicsEngine へ送り、
// FishEyePass が定数バッファとして受け取る。
//
// 掛かるのはシーンの絵だけで、UI には掛からない
// (UIパスはポストプロセスより後で AfterTAAColor へ直接描かれるため)。
// UIを曲げたいときは UIBase の湾曲(CurveAngle)を使うこと
//==========================================================================================
struct FishEyeComponent
{
	// ---- 設定(保存される) ----

	// 歪みの中心(UV : 画面左上が {0,0}、右下が {1,1})。既定は画面中央
	Math::Vector2 center	= { 0.5f, 0.5f };

	// 歪みの強さ。0で歪まない。
	// 0.2 前後でうっすら、0.5 を超えると四隅の黒がはっきり見えてくる
	float strength			= 0.2f;

	// false なら歪ませずそのまま通す
	bool  enable			= false;
};

template<>
struct Engine::ECS::ComponentTraits<FishEyeComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		FishEyeComponent& _comp = Engine::Editor::GetValue<FishEyeComponent>(a_pData);
		a_ar.Field("center",   _comp.center);
		a_ar.Field("strength", _comp.strength);
		a_ar.Field("enable",   _comp.enable);
	}

	static void Edit(CompEditContext& a_context)
	{
		FishEyeComponent& _comp = Engine::Editor::GetValue<FishEyeComponent>(a_context.pData);

		ImGui::Checkbox("FishEye Enable", &_comp.enable);
		ImGui::DragFloat2("Center (UV)", &_comp.center.x, 0.01f, 0.0f, 1.0f);
		ImGui::TextDisabled("画面左上が 0,0 / 右下が 1,1");
		ImGui::DragFloat("Strength", &_comp.strength, 0.01f, -1.0f, 2.0f);
		ImGui::TextDisabled("正で樽型(四隅が黒くなる) / 負で糸巻き型");
	}
};
