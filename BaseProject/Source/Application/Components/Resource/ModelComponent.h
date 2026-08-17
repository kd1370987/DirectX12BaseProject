#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../../../Engine/Editor/Helper/EditorHelper.h"

#include "../../../Engine/ECS/World/World.h"

struct ModelComponent
{
	Math::Color colorScale = { 1.0f,1.0f,1.0f,1.0f };
	Math::Vector3 emissiveScale = { 1.0f,1.0f,1.0f };

	//--------------------------------------------------------------------------
	// 自己発光（ブルームで光らせたいとき用）
	//
	// emissiveScale は「エミッシブテクスチャに掛ける倍率」なので、
	// テクスチャを持たないモデル(黒テクスチャにフォールバックする)は
	// 何倍しても 0 のままで絶対に光らない。
	// こちらはテクスチャもマテリアルの emissive も要らない加算の発光。
	//
	//   実際にGBufferへ書かれる値 = emissiveColor * emissiveIntensity
	//
	// 色は 0〜1 のまま扱い、1.0 超えは強度側で作る。
	// ブルームは輝度がしきい値(既定1.0)を超えた画素だけを拾うので、
	// 光らせたいなら強度をしきい値より大きくすること。
	// 既定は強度0＝オフなので、設定しなければ今までと同じ見た目になる。
	//--------------------------------------------------------------------------
	Math::Vector3 emissiveColor = { 1.0f,1.0f,1.0f };	// 発光色(0〜1)
	float emissiveIntensity = 0.0f;							// 発光の強さ(上限なし / 0でオフ)

	// モデル参照用
	Engine::Handle<Engine::Resource::Model> handle = {};	// ランタイム用
	Engine::GUID modelGUID = {};									// 記録用

	// シェーダーへ送る実効的な自己発光
	Math::Vector3 GetEmissiveAdd() const
	{
		return {
			emissiveColor.x * emissiveIntensity,
			emissiveColor.y * emissiveIntensity,
			emissiveColor.z * emissiveIntensity
		};
	}
};

template<>
struct Engine::ECS::ComponentTraits<ModelComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ModelComponent& _comp = Engine::Editor::GetValue<ModelComponent>(a_pData);
		a_ar.Field("colorScale", _comp.colorScale);
		a_ar.Field("emissiveScale", _comp.emissiveScale);
		a_ar.Field("emissiveColor", _comp.emissiveColor);
		a_ar.Field("emissiveIntensity", _comp.emissiveIntensity);
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
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
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

		// ---------------------------------------------------------
		// 色まわり
		//
		// ColorPicker(常時展開)ではなく ColorEdit(色見本をクリックでピッカー)を使う。
		// インスペクタが縦に短くなるのと、こちらはドラッグや右クリックでの
		// 数値入力にも対応していて、色の指定方法を選べるため。
		//
		// ※ emissiveScale / emissiveColor は Math::Vector3(3成分)なので必ず3成分版を使うこと。
		//    ColorEdit4 にすると float 4個ぶん書き戻され、
		//    直後にあるメンバをアルファ値で潰してしまう。
		// ---------------------------------------------------------
		ImGui::ColorEdit4("ColorScale", _comp.colorScale.Data());
		ImGui::ColorEdit3("EmissiveScale", (float*)&_comp.emissiveScale.x);

		// ---------------------------------------------------------
		// 自己発光（ブルーム用）
		//
		// 色は 0〜1 のピッカーで選び、1.0 超えは強度側で作る。
		// ピッカー自体は 0〜1 しか扱えないので、HDRの明るさは
		// 「色 × 強度」に分けるのが結局いちばん触りやすい。
		// ---------------------------------------------------------
		ImGui::SeparatorText("Emissive (Bloom)");

		ImGui::ColorEdit3("Emissive Color", (float*)&_comp.emissiveColor.x);

		// 上限なし。ブルームのしきい値(既定1.0)を超えるまで上げると光り出す
		ImGui::DragFloat("Emissive Intensity", &_comp.emissiveIntensity, 0.05f, 0.0f, FLT_MAX);

		// 実際にシェーダーへ渡る値。しきい値を超えているかの目安になる
		const Math::Vector3 _emissiveAdd = _comp.GetEmissiveAdd();
		ImGui::TextDisabled("-> (%.2f, %.2f, %.2f)", _emissiveAdd.x, _emissiveAdd.y, _emissiveAdd.z);
	}
};