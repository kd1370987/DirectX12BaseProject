#pragma once

#include "Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	//======================================================================================
	// 空間のチリ
	//
	// 空気中の細かいゴミを1つのエフェクトで出し、それをカメラに追従させて
	// 「どこへ行っても空間にチリが漂っている」ように見せるためのもの。
	//
	// 実際に出すのは EffectAsset なので、絵の中身はアセット側で作る。
	// ここが持つのは「どこに・どれだけの広さで出すか」と「どうカメラに付いていくか」だけ。
	//
	// ・エンティティは1つだけ作って動かし続ける(毎フレーム出し直さない)。
	//   出し直すとパーティクルの発生源の席を取り直すことになるうえ、
	//   粒が毎回リセットされて流れが途切れる。
	//======================================================================================
	struct Dast
	{
		// 出すエフェクト。未設定ならチリは出ない
		Engine::GUID effectGUID = Engine::DefaultGUID;

		Engine::ResourceRef<Engine::Resource::EffectAsset> m_effectAsset = {};		// エフェクト
		Math::Color m_colorScale = { 1.0f, 1.0f, 1.0f, 1.0f };						// 色スケール

		// 座標ボリューム
		//
		// center は追従の結果なので保存しない(読み込み後の最初のフレームで
		// カメラの位置へ置き直される)。
		// scale はエフェクト全体の倍率として渡すので、粒の大きさにも掛かる。
		// 広さだけ変えたいときは、アセット側の positionRadius を基準にして
		// ここは 1 のままにしておくとよい
		Math::Vector3 center = {};			// 中心座標 : カメラに追従予定
		float scale = 1.0f;					// 出現空間スケール

		// 追従
		//
		// カメラが center から length より離れたら、speed でそこまで詰める。
		// 詰め切るのは「length だけ離れたところ」までなので、
		// チリの塊はカメラの後ろを引きずるように付いてくる。
		// length を 0 にすると常にカメラへ張り付く
		float length = 10.0f;				// この距離離れたら追従する
		float speed = 20.0f;				// 追従スピード
	};

	/// <summary>
	/// 空間のチリ : カメラに追従するエフェクトを1つ出し続ける
	/// </summary>
	/// <remarks>
	/// もとは SceneAmbientObject として環境光・平行光・フォグ・空と一緒に持っていたが、
	/// それらはシーン(Engine::Scene::SceneAmbient)の持ち物へ移した。
	/// チリはレンダーパスの設定ではなく ECS のエンティティ(エフェクト)を出すものなので、
	/// 出したエンティティの始末まで面倒を見るオブジェクトとして残してある。
	/// </remarks>
	class AmbientDustObject : public Engine::GameObject::BaseObject
	{
	public:

		// 更新処理 : チリをカメラへ追従させる
		//
		// GameObject の Update は ECS の全フェーズが終わった後に走る。
		// カメラのワールド行列はもう確定しているので、今フレームの位置で追従できる。
		// (代わりに CalcMatrixSystem はもう通り過ぎているので、
		//  チリの行列はこちらで直接入れる)
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 解放処理 : 出しているチリを片付ける
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "AmbientDust"; }

		// インスペクター : チリの設定
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// インスペクターのチリの欄
		void DrawDastInspector(Engine::GameObject::ObjectContext& a_context);

		// メインカメラのワールド座標を取る。居なければ false
		bool TryGetMainCameraPos(
			Engine::GameObject::ObjectContext& a_context, Math::Vector3& a_outPos) const;

		// チリのエンティティが無ければ作る(設定が変わっていたら作り直す)
		void EnsureDastEntity(Engine::GameObject::ObjectContext& a_context);

		// 追従の結果(center と scale)をチリのエンティティへ書き込む
		void ApplyDastToEntity(Engine::GameObject::ObjectContext& a_context);

		// チリのエンティティを解放予約して手放す
		void ReleaseDastEntity(Engine::GameObject::ObjectContext& a_context);

	private:

		//-------------------------------------------------------------------
		// 設定(保存される)
		//-------------------------------------------------------------------
		// カメラに追従するチリ
		Dast m_dast = {};

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		// 出しているチリのエンティティ。生きていない間は INVALID
		Engine::ECS::Entity m_dastEntity = Engine::ECS::Limits::INVALID_ENTITY;

		// チリのエンティティを作ったときのエフェクト。
		// 設定を差し替えられたらこれとの差で気付いて作り直す
		Engine::GUID m_dastSpawnedGUID = Engine::DefaultGUID;

		// center をカメラの位置へ合わせ終わったか。
		// 立つまでは追従させず、その場でカメラへ置く
		// (読み込み直後に原点からカメラまで長い距離をなめてしまうため)
		bool m_isDastCentered = false;
	};
}
