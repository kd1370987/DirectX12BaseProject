#pragma once

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"

//==========================================================================================
// PointLightComponent
//
// このエンティティの位置を光源にする点光源。
//
// ・実体を持たずハンドルだけ持つ理由
//     ライトの実体は GraphicsEngine の LightManager が1本のプールで持っていて、
//     毎フレームそこから GPU のバッファへ詰め直される。
//     コンポーネント側が実体を持つとプールと二重管理になるので、
//     ここは「席の予約票(ハンドル)」と、そこへ書き込む設定値だけを持つ。
//
// ・ハンドルを保存しない理由
//     プールの添字はシーンを読み直すたびに振り直される。保存すると
//     他人の席を指したまま復元されるので、席は毎回 PointLightSystem が取り直す。
//
// ・posOffset を持つ理由
//     機体のバーニアやミサイルの尾など、エンティティの原点そのものではない
//     場所を光らせたいことが多い。エンティティを1つ増やさずに済ませるため、
//     取り付け位置だけをここが持つ(BoosterEffectComponent と同じ考え方)。
//
// ・brightness の桁
//     逆二乗で減衰した後の値なので、見た目が出るまで数十のオーダーが要る。
//     色は 0〜1 のピッカーで選び、明るさは別の数値で作る
//     (ModelComponent のエミッシブと同じ分け方)。
//==========================================================================================
struct PointLightComponent
{
	// ---- 設定値 ----
	// このエンティティの行列を基準にしたローカル位置
	Math::Vector3 posOffset = { 0.0f, 0.0f, 0.0f };

	Math::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };	// 色(0〜1)
	float brightness = 50.0f;						// 明るさ : color に掛かる
	float range = 15.0f;							// 光の届く距離。ここで減衰が0になる

	// ---- ランタイム(保存しない) ----
	// プールの席。PointLightSystem が無効なら取り直す
	Engine::Handle<Engine::Graphics::PointLight> handle = {};
};

template<>
struct Engine::ECS::ComponentTraits<PointLightComponent>
{
	//----------------------------------------------------------------------------------
	// 借りている席を返す
	//
	// コンポーネントはデストラクタが走らないので、返すのはここの仕事。
	// 返し忘れるとシーンを読み直すたびに席が減り、最後は上限に達して
	// 新しいライトが1つも点かなくなる。
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData)
	{
		PointLightComponent& _comp = Engine::Editor::GetValue<PointLightComponent>(a_pData);
		if (!_comp.handle.IsValid()) return;

		// 終了処理の順によっては、こちらが先に消えていることがある
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;

		_pGE->RefLightManager()->RemoveLight(_comp.handle);
		_comp.handle = {};
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PointLightComponent& _comp = Engine::Editor::GetValue<PointLightComponent>(a_pData);

		a_ar.Field("posOffset", _comp.posOffset);
		a_ar.Field("color", _comp.color);
		a_ar.Field("brightness", _comp.brightness);
		a_ar.Field("range", _comp.range);
	}

	static void Edit(CompEditContext& a_context)
	{
		PointLightComponent& _comp = Engine::Editor::GetValue<PointLightComponent>(a_context.pData);

		ImGui::SeparatorText("Mount");
		ImGui::TextDisabled("このエンティティの行列基準。原点以外を光らせたいときに使う");
		ImGui::DragFloat3("PosOffset", &_comp.posOffset.x, 0.01f);

		ImGui::SeparatorText("Light");
		// 色は 0〜1 のピッカーで選び、1.0 超えの明るさは Brightness 側で作る
		// (ピッカー自体が 0〜1 しか扱えないため)
		ImGui::ColorEdit3("Color", _comp.color.Data());

		// 逆二乗で減衰した後に効くので、見た目が出るまで数十は要る
		ImGui::DragFloat("Brightness", &_comp.brightness, 0.5f, 0.0f, FLT_MAX);

		// これを超えた先は計算ごと飛ばされる。大きくするほど拾うピクセルが増える
		ImGui::DragFloat("Range", &_comp.range, 0.1f, 0.0f, FLT_MAX);
		if (_comp.range <= 0.0f)
		{
			ImGui::TextDisabled("0 : どこも照らさない");
		}

		// ランタイムは表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Handle : %s", _comp.handle.IsValid() ? "取得済み" : "未取得(上限かも)");
	}
};
