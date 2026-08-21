#pragma once

//==========================================================================================
//
// EffectAsset を「その場に出す」ための小さなヘルパー。
//
// プレハブを介さずに、エフェクト再生に必要な最小限のエンティティを組んで出す。
//   LocalTransform / WorldMatrix / EffectAssetComponent / EffectComponent
//
// 出したエンティティは destroyOnFinish で自分から消えるので、
// 呼んだ側は後片付けを気にしなくてよい
// (出しっぱなしのパーツを含むエフェクトを渡すと消えないので注意)。
//
// 生成はシステム反復中に即時に行えない(アーキタイプが壊れる)ため、
// World の遅延生成コマンドに積む。実体化は次の BeginFrame。
//
//==========================================================================================

namespace Engine
{
	namespace ECS { class World; }
}

namespace App::Utility
{
	/// <summary>
	/// エフェクトアセットを指定座標で再生する(遅延コマンドを積むだけ)
	/// </summary>
	/// <param name="a_effectGUID">再生するエフェクト。未設定なら何もしない</param>
	/// <param name="a_pos">再生位置(ワールド)</param>
	/// <param name="a_isDestroyOnFinish">
	/// 出し切ったら自分ごと消えるか。
	/// 撃ちっぱなしにする通常の演出は true。
	/// エディターのプレビューのように「同じものを何度も再生し直したい」場合は false にして、
	/// 出したエンティティの寿命を呼んだ側が握る
	/// </param>
	/// <param name="a_emitDir">
	/// 吹き出す向き(ワールド)。0ベクトルならアセットのパーツが持っている向きのまま出す。
	/// 出す場所は同じでも向きだけ変えたいもの(ブースターのスパークなど)向け
	/// </param>
	/// <param name="a_scale">
	/// エフェクト全体の大きさ倍率。アセットは GUID 単位で共有されるので、
	/// 同じ絵を大小で使い分けたいときはアセットを増やさずここで付ける
	/// </param>
	/// <returns>生成コマンドを積めたら true</returns>
	bool SpawnEffectAt(
		Engine::ECS::World& a_world,
		const Engine::GUID& a_effectGUID,
		const Math::Vector3& a_pos,
		bool a_isDestroyOnFinish = true,
		const Math::Vector3& a_emitDir = {},
		float a_scale = 1.0f);
}
