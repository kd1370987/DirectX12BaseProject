#include "ParticleEdit.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../../AssetLink.h"

#include "../../../../../../Editor/Helper/EditorHelper.h"
#include "../../../../../../Resource/Data/Texture/IO/TextureIO.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// パーティクルアセットの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void ParticleEdit(
		const ECS::EngineServices& a_services,
		const Engine::GUID& a_guid,
		Resource::ParticlesAsset* a_pParticles,
		EditorContext* a_pEditContext)
	{
		if (!a_pParticles) { return; }

		if (ImGui::Button("Save") && a_services.pAssetDatabase)
		{
			// ファイルパス取得 : 呼び出し側が選んでいるGUIDから引く
			auto _filePath = a_services.pAssetDatabase->GetFilePathFromGUID(a_guid);
			a_pParticles->Save(_filePath);
			ENGINE_LOG("%s: Save Particles", _filePath.c_str());
		}

		// パラメーター変更
		Engine::Editor::Field("Name", a_pParticles->RefName());
		Engine::Editor::Text("%s", a_guid.String().c_str());

		Engine::Editor::Line();

		ImGui::PushID(1);
		Engine::Editor::Header("InitialSpeed");
		Engine::Editor::Field("Min", a_pParticles->RefInitalSpeedMin(), 0.1f, 0.0f);
		Engine::Editor::Field("Max", a_pParticles->RefInitalSpeedMax(), 0.1f, 0.0f);
		ImGui::PopID();

		Engine::Editor::Line();

		// 下限を 0 にしない : 負の値で浮き上がらせたいことがある(煙・炎)
		Engine::Editor::Field("GravityPow", a_pParticles->RefGravityPow(), 0.05f);
		Engine::Editor::Tooltip("1 で普通に落ちる / 0 で無重力 / 負で浮き上がる");

		// ---- 空気抵抗 ----
		// 勢いよく飛び出して失速する動き。爆発の破片や煙はこれが無いと
		// 最後まで等速で飛んでいってしまう
		Engine::Editor::Field("Drag (/s)", a_pParticles->RefDrag(), 0.05f, 0.0f);
		if (a_pParticles->GetDrag() <= 0.0f)
		{
			Engine::Editor::HelpText("0 : 減速しない(等速で飛び続ける)");
		}
		else
		{
			Engine::Editor::HelpText("大きいほど早く失速する(爆発の破片なら 2〜5 が目安)");
		}

		Engine::Editor::Line();

		ImGui::PushID(2);
		Engine::Editor::Header("LifeTime");
		Engine::Editor::Field("Min", a_pParticles->RefLifeTimeMin(), 0.1f, 0.0f);
		Engine::Editor::Field("Max", a_pParticles->RefLifeTimeMax(), 0.1f, 0.0f);
		ImGui::PopID();

		Engine::Editor::Line();

		Engine::Editor::Field("Capacity", a_pParticles->RefCapacity(), 1, 0);
		Engine::Editor::Field("EmissionRate", a_pParticles->RefEmissionRate(), 1, 0);

		Engine::Editor::Header("Over Lifetime");
		Engine::Editor::HelpText("寿命のどこまで進んだかで、サイズと色を動かす");

		// ---- サイズの変化 ----
		Engine::Editor::Field("EndSizeScale", a_pParticles->RefEndSizeScale(), 0.05f, 0.0f);
		Engine::Editor::Tooltip("寿命の終わりでのサイズ倍率。煙は 1 より大きく、火花は小さく");

		// ---- 色の変化 ----
		// RGB は 1 を超えてよい。超えたぶんがブルームのしきい値を抜けて光る
		Field("StartColor", a_pParticles->RefStartColor());
		Field("EndColor", a_pParticles->RefEndColor());
		Engine::Editor::Tooltip("RGB は 1 を超えてよい(超えたぶんが光る)。爆発は白→橙→暗い煙");

		// ---- フェード ----
		// 寿命に対する割合で持つので、粒ごとに寿命がばらついても見え方が揃う
		Engine::Editor::Field("FadeIn (ratio)", a_pParticles->RefFadeInRatio(), 0.01f, 0.0f, 1.0f);
		Engine::Editor::Field("FadeOut (ratio)", a_pParticles->RefFadeOutRatio(), 0.01f, 0.0f, 1.0f);
		if (a_pParticles->GetFadeInRatio() + a_pParticles->GetFadeOutRatio() > 1.0f)
		{
			Engine::Editor::HelpText("合計が 1 を超えています(不透明になりきる前に消え始めます)");
		}

		// ---- どの座標系で回すか ----
		// ワールドのままだと、発生源が横へ動いた瞬間に出した粒だけ置き去りになる。
		// 噴射のように発生源へくっついてほしいものは Local
		Engine::Editor::Header("Simulation");
		Field("SimulationSpace", a_pParticles->RefSimulationSpace());
		if (a_pParticles->IsLocalSpace())
		{
			Engine::Editor::HelpText("発生源にくっついて動く(ブースターの噴射など)");
			Engine::Editor::HelpText("※ 重力は発生源のローカル軸に掛かるので GravityPow は 0 推奨");
			Engine::Editor::HelpText("※ 同じアセットを同時に使える発生源は 8 個まで");
		}
		else
		{
			Engine::Editor::HelpText("出したその場に残る(煙・爆発・弾の軌跡など)");
		}

		// ---- 色の重ね方 ----
		// 加算は光り物、半透明は煙や破片。
		// 加算のまま煙を出すと背景ごと明るくなってしまう
		Engine::Editor::Header("Blend");
		Field("BlendMode", a_pParticles->RefBlendMode());
		if (a_pParticles->GetBlendMode() == Particle::EParticleBlendMode::Additive)
		{
			Engine::Editor::HelpText("重ねるほど明るくなる。火花・炎・爆発の芯向き");
		}
		else
		{
			Engine::Editor::HelpText("背景を明るくしない。煙・破片向き");
			Engine::Editor::HelpText("※ 粒の前後は並べ替えていないので、重なりが入れ替わって見えることがあります");
		}

		Engine::Editor::Line();

		// ---- 板ポリの向き ----
		// 進行方向に画像を回すかどうか。Billboard 以外のとき Stretch が効く
		Field("Orientation", a_pParticles->RefOrientation());
		if (a_pParticles->GetOrientation() == Particle::EParticleOrientation::Billboard)
		{
			Engine::Editor::HelpText("Always faces camera (texture up = screen up)");
		}
		else
		{
			Engine::Editor::HelpText("Texture up (V=0) points along velocity");
			Engine::Editor::Field("Stretch", a_pParticles->RefStretch(), 0.05f, 0.01f);
		}

		Engine::Editor::Line();

		// 現在選択されているテクスチャ
		const auto* _pTex = a_services.pResourceManager->Ref(a_pParticles->GetTexHandle());

		Engine::Editor::Line();

		// テクスチャ選択コンボボックス
		// 反映は専用のロード関数を通すので、選択だけを共通ヘルパーに任せる
		GUID _selectedGUID = {};
		if (Editor::AssetPicker(
			a_services,
			"SelectTexture",
			"Texture",
			a_pParticles->GetTexGUID(),
			_selectedGUID))
		{
			// テクスチャのハンドル取得
			// ロードされていなかったら止まる
			const auto _context = Resource::MakeManagerOnlyContext(a_services.pResourceManager, a_services.pAssetDatabase);
			a_pParticles->SetTexture(_selectedGUID, Resource::TextureIO::LoadTexture(_selectedGUID, TexColor::WHITE, &_context));
		}

		// 今指しているテクスチャ。押せばテクスチャのインスペクターへ飛べる
		DrawAssetLink(a_pEditContext, "Texture :", a_pParticles->GetTexGUID());

		// テクスチャの画像を表示
		if (_pTex)
		{
			auto _gpuHandle = EditorHelper::GetImGuiTexHandle(_pTex->GetImGuiSRV());
			EditorHelper::DrawSRVView(_gpuHandle, static_cast<float>(_pTex->GetDesc().Width), static_cast<float>(_pTex->GetDesc().Height));
		}
	}
}
