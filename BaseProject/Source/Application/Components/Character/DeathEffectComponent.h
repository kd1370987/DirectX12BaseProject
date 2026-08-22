#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorHelper.h"
#include "Engine/Editor/Helper/EditorHelper.inl"

//==========================================================================================
// DeathEffectComponent
//
// 死んだときに出すエフェクトを登録するコンポーネント。
// キャラクター(プレイヤー・敵)でも弾でも、消えるときに何か出したいものに付ける。
//
// ・持つのは EffectAsset(パーティクル+メッシュを1枚にまとめたもの)だけ。
//   時間差もパーツごとの EffectTiming(startDelay / duration)で表せるので、
//   演出をプレハブへ組み立てる必要はない。出し切ったら自分から消える。
// ・出すのは DeathEffectSystem。「誰がどこで死んだか」は DeathEventResource 経由で受け取る。
//   死因を持っているシステム(体力切れなら HealthSystem、着弾なら ExplodeOnHitSystem)は
//   死亡を積むだけでよく、エフェクトの存在を知らなくて済む。
// ・以前はプレハブも登録できたが、演出のためだけにプレハブを1枚挟むと
//   「どちらが入っているのか」を出す側が種別で見分ける必要があり、
//   ExplosionComponent のような中継のコンポーネントも要った。EffectAsset に一本化してある。
// ・ハンドルは PostDeserialize(EffectFixupSystem)で GUID から解決される。
//   死んだ瞬間に読み込みが走らないよう、先に解決しておくためのもの。
//==========================================================================================
struct DeathEffectComponent
{
	// 出すエフェクト(未設定なら何も出ない)
	Engine::GUID effectGUID = Engine::DefaultGUID;

	// GUIDから解決したアセットのハンドル(EffectFixupSystem が入れる)
	Engine::Handle<Engine::Resource::EffectAsset> effectHandle = {};
};

template<>
struct Engine::ECS::ComponentTraits<DeathEffectComponent>
{
	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事。
	// ECS がエンティティを消すとき・コンポーネントを外すとき・
	// PostDeserialize へ入り直すとき(fixup が取り直す)に必ず呼ぶ。
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData)
	{
		DeathEffectComponent& _comp = Engine::Editor::GetValue<DeathEffectComponent>(a_pData);
		auto& _resourceManager = Engine::Resource::ResourceManager::Instance();

		_resourceManager.ReleaseHandle(_comp.effectHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		DeathEffectComponent& _comp = Engine::Editor::GetValue<DeathEffectComponent>(a_pData);
		a_ar.Field("effectGUID", _comp.effectGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		DeathEffectComponent& _comp = Engine::Editor::GetValue<DeathEffectComponent>(a_context.pData);

		Engine::Editor::EditorHelper::DrawAssetSelectCombo<Engine::Resource::EffectAsset>(
			"Death Effect",
			"EffectAsset",
			_comp.effectGUID,
			_comp.effectHandle);

		if (_comp.effectGUID == Engine::DefaultGUID)
		{
			ImGui::TextDisabled("(未設定 : 死んでも何も出ない)");
			return;
		}

		// 中身の確認用。細かい編集はアセット側のインスペクターで行う
		const auto* _pEffect = Engine::Resource::ResourceManager::Instance().Get(_comp.effectHandle);
		if (!_pEffect)
		{
			ImGui::TextDisabled("(読み込み中 / 見つからないアセット)");
			return;
		}

		ImGui::TextDisabled("Particle Parts : %d / Mesh Parts : %d",
			static_cast<int>(_pEffect->GetParticleParts().size()),
			static_cast<int>(_pEffect->GetMeshParts().size()));
	}
};
