#include "ParticleEdit.h"

#include "../../AssetLink.h"

#include "../../../../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../../../../../Resource/Data/Texture/IO/TextureIO.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// パーティクルアセットの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void ParticleEdit(Resource::ParticlesAsset* a_pParticles, EditorContext* a_pEditContext)
	{
		if (!a_pParticles) { return; }

		if (ImGui::Button("Save"))
		{
			// ファイルパス取得
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_pParticles->GetGUID());
			a_pParticles->Save(_filePath);
			MainEditor::Instance().AddLog("%s", _filePath.c_str());
			MainEditor::Instance().AddLog(" : Save Particles\n");
		}

		// パラメーター変更
		ImGui::InputText("Name", &a_pParticles->RefName());
		ImGui::Text("%s", a_pParticles->GetGUID().String().c_str());

		ImGui::Separator();

		ImGui::PushID(1);
		ImGui::Text("InitialSpeed");
		ImGui::DragFloat("Min", &a_pParticles->RefInitalSpeedMin(), 0.1f, 0.0f);
		ImGui::DragFloat("Max", &a_pParticles->RefInitalSpeedMax(), 0.1f, 0.0f);
		ImGui::PopID();

		ImGui::Separator();

		// 下限を 0 にしない : 負の値で浮き上がらせたいことがある(煙・炎)
		ImGui::DragFloat("GravityPow", &a_pParticles->RefGravityPow(), 0.05f);
		ImGui::TextDisabled("1 で普通に落ちる / 0 で無重力 / 負で浮き上がる");

		// ---- 空気抵抗 ----
		// 勢いよく飛び出して失速する動き。爆発の破片や煙はこれが無いと
		// 最後まで等速で飛んでいってしまう
		ImGui::DragFloat("Drag (/s)", &a_pParticles->RefDrag(), 0.05f, 0.0f);
		if (a_pParticles->GetDrag() <= 0.0f)
		{
			ImGui::TextDisabled("0 : 減速しない(等速で飛び続ける)");
		}
		else
		{
			ImGui::TextDisabled("大きいほど早く失速する(爆発の破片なら 2〜5 が目安)");
		}

		ImGui::Separator();

		ImGui::PushID(2);
		ImGui::Text("LifeTime");
		ImGui::DragFloat("Min", &a_pParticles->RefLifeTimeMin(), 0.1f, 0.0f);
		ImGui::DragFloat("Max", &a_pParticles->RefLifeTimeMax(), 0.1f, 0.0f);
		ImGui::PopID();

		ImGui::Separator();

		ImGui::DragInt("Capacity", &a_pParticles->RefCapacity(), 1, 0);
		ImGui::DragInt("EmissionRate", &a_pParticles->RefEmissionRate(), 1, 0);

		ImGui::SeparatorText("Over Lifetime");
		ImGui::TextDisabled("寿命のどこまで進んだかで、サイズと色を動かす");

		// ---- サイズの変化 ----
		ImGui::DragFloat("EndSizeScale", &a_pParticles->RefEndSizeScale(), 0.05f, 0.0f);
		ImGui::TextDisabled("寿命の終わりでのサイズ倍率。煙は 1 より大きく、火花は小さく");

		// ---- 色の変化 ----
		// RGB は 1 を超えてよい。超えたぶんがブルームのしきい値を抜けて光る
		EditorHelper::DrawColorEdit("StartColor", a_pParticles->RefStartColor());
		EditorHelper::DrawColorEdit("EndColor", a_pParticles->RefEndColor());
		ImGui::TextDisabled("RGB は 1 を超えてよい(超えたぶんが光る)。爆発は白→橙→暗い煙");

		// ---- フェード ----
		// 寿命に対する割合で持つので、粒ごとに寿命がばらついても見え方が揃う
		ImGui::DragFloat("FadeIn (ratio)", &a_pParticles->RefFadeInRatio(), 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("FadeOut (ratio)", &a_pParticles->RefFadeOutRatio(), 0.01f, 0.0f, 1.0f);
		if (a_pParticles->GetFadeInRatio() + a_pParticles->GetFadeOutRatio() > 1.0f)
		{
			ImGui::TextDisabled("合計が 1 を超えています(不透明になりきる前に消え始めます)");
		}

		ImGui::Separator();

		// ---- どの座標系で回すか ----
		// ワールドのままだと、発生源が横へ動いた瞬間に出した粒だけ置き去りになる。
		// 噴射のように発生源へくっついてほしいものは Local
		ImGui::Text("Simulation");
		EditorHelper::DrawEnumCombo("SimulationSpace", a_pParticles->RefSimulationSpace());
		if (a_pParticles->IsLocalSpace())
		{
			ImGui::TextDisabled("発生源にくっついて動く(ブースターの噴射など)");
			ImGui::TextDisabled("※ 重力は発生源のローカル軸に掛かるので GravityPow は 0 推奨");
			ImGui::TextDisabled("※ 同じアセットを同時に使える発生源は 8 個まで");
		}
		else
		{
			ImGui::TextDisabled("出したその場に残る(煙・爆発・弾の軌跡など)");
		}

		ImGui::Separator();

		// ---- 色の重ね方 ----
		// 加算は光り物、半透明は煙や破片。
		// 加算のまま煙を出すと背景ごと明るくなってしまう
		ImGui::Text("Blend");
		EditorHelper::DrawEnumCombo("BlendMode", a_pParticles->RefBlendMode());
		if (a_pParticles->GetBlendMode() == Particle::EParticleBlendMode::Additive)
		{
			ImGui::TextDisabled("重ねるほど明るくなる。火花・炎・爆発の芯向き");
		}
		else
		{
			ImGui::TextDisabled("背景を明るくしない。煙・破片向き");
			ImGui::TextDisabled("※ 粒の前後は並べ替えていないので、重なりが入れ替わって見えることがあります");
		}

		ImGui::Separator();

		// ---- 板ポリの向き ----
		// 進行方向に画像を回すかどうか。Billboard 以外のとき Stretch が効く
		ImGui::Text("Orientation");
		EditorHelper::DrawEnumCombo("Orientation", a_pParticles->RefOrientation());
		if (a_pParticles->GetOrientation() == Particle::EParticleOrientation::Billboard)
		{
			ImGui::TextDisabled("Always faces camera (texture up = screen up)");
		}
		else
		{
			ImGui::TextDisabled("Texture up (V=0) points along velocity");
			ImGui::DragFloat("Stretch", &a_pParticles->RefStretch(), 0.05f, 0.01f);
		}

		ImGui::Separator();

		// 現在選択されているテクスチャ
		const auto* _pTex = Resource::ResourceManager::Instance().Ref(a_pParticles->GetTexHandle());

		ImGui::Separator();

		// テクスチャ選択コンボボックス
		// 反映は専用のロード関数を通すので、選択だけを共通ヘルパーに任せる
		GUID _selectedGUID = {};
		if (Editor::EditorHelper::DrawAssetGUIDCombo(
			"SelectTexture",
			"Texture",
			a_pParticles->GetTexGUID(),
			_selectedGUID))
		{
			// テクスチャのハンドル取得
			// ロードされていなかったら止まる
			a_pParticles->SetTexture(_selectedGUID, Resource::TextureIO::LoadTexture(_selectedGUID, TexColor::WHITE));
		}

		// 今指しているテクスチャ。押せばテクスチャのインスペクターへ飛べる
		DrawAssetLink(a_pEditContext, "Texture :", a_pParticles->GetTexGUID());

		// テクスチャの画像を表示
		if (_pTex)
		{
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			EditorHelper::DrawSRVView(_gpuHandle, static_cast<float>(_pTex->GetDesc().Width), static_cast<float>(_pTex->GetDesc().Height));
		}
	}
}
