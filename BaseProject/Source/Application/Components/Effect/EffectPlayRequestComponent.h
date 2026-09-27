#pragma once

//==========================================================================================
// EffectPlayRequestComponent
//
// エフェクトを再生させたいか。制御側のシステム(Thruster / Ballistic / MuzzleFlash など)が書き、
// EffectUpdateSystem が立ち上がり・立ち下がりで再生と停止をする。
//
// ・以前は EffectAssetComponent::isPlay として、アセット参照・進行状態と同じ型に入っていた。
//   要求を出すだけの側まで進行状態の書き手とぶつかっていた。
// ・EffectAssetComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない
//   (生成時の値は EffectFixupSystem が playOnStart から入れる)。
//==========================================================================================
struct EffectPlayRequestComponent
{
	bool isPlay = false;	// 再生させたいか
};

template<>
struct Engine::ECS::ComponentTraits<EffectPlayRequestComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		EffectPlayRequestComponent& _comp = Engine::Editor::GetValue<EffectPlayRequestComponent>(a_context.pData);
		Engine::Editor::Field("IsPlay", _comp.isPlay);
	}
};
