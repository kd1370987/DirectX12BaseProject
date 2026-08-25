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

	/// <summary>
	/// エフェクトアセットを指定座標で再生し、作ったエンティティを返す(即時生成)
	/// </summary>
	/// <remarks>
	/// SpawnEffectAt との違いは「その場で作って ID を返す」ところだけ。
	/// 出したあとも位置を動かし続けたい・こちらの都合で消したい、という
	/// 相手を握っておきたい場面(環境のチリなど)向け。
	///
	/// ただし即時生成はチャンクの並びを変えるので、
	/// **ECS の反復中に呼んではいけない**。
	/// 呼んでよいのはシステムの外 : GameObject の Update / Draw など。
	/// システムの中から出すときは今までどおり SpawnEffectAt(遅延)を使うこと。
	///
	/// 返ったエンティティは PostDeserialize から始まるので、
	/// アセットの解決(EffectFixupSystem)が済むのは次の BeginFrame。
	/// それまでは何も出ないが、位置は入れておいてよい。
	/// </remarks>
	/// <returns>作ったエンティティ。作れなければ INVALID_ENTITY</returns>
	Engine::ECS::Entity SpawnEffectAtNow(
		Engine::ECS::World& a_world,
		const Engine::GUID& a_effectGUID,
		const Math::Vector3& a_pos,
		bool a_isDestroyOnFinish = true,
		const Math::Vector3& a_emitDir = {},
		float a_scale = 1.0f);
}
