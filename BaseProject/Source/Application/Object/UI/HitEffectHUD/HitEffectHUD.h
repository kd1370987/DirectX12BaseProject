#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// 自分の撃った弾が当たったときに出す手応え(ヒットマーカー)。
	///
	/// クロスマークのUIを一定時間出しつつ、同時にヒット音を鳴らす。
	///
	/// ・ヒットは HitEventResource(そのフレームのヒット全部)を見る。弾は当たった直後に
	///   消えるので弾側のコンポーネントでは間に合わない。「誰が撃った弾か」は
	///   HitEvent.shooter に入っているので、プレイヤーが撃ったものだけを拾う。
	/// ・当てた相手が ScoreTargetComponent を持っているときだけ反応する。
	///   壁や地面に当てても手応えが出ると、当たった合図として意味を成さないため。
	/// ・GameObjectManager::Update は Physics(ヒットを積む)より後、
	///   HitEventClearSystem(次フレーム PreUpdate で消す)より前なので、
	///   このタイミングなら同じフレームのヒットがそのまま読める。
	/// ・テクスチャと音はインスペクターから選ぶ。
	/// </summary>
	class HitEffectHUD : public UIBase
	{
	public:

		// 初期化処理 : テクスチャとサウンドインスタンスの用意
		void Init(Engine::GameObject::ObjectContext& a_context) override;

		// 解放処理 : サウンドインスタンスを返す
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理 : 自分の弾のヒットを拾って表示時間を巻き戻す
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理 : 表示時間が残っている間だけ描く
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "HitEffectHUD"; }

		// インスペクター
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// ヒットに反応する(表示時間の巻き戻しと発音)
		void OnHit(Engine::GameObject::ObjectContext& a_context);

		// サウンドインスタンスを取り直す
		void RequestSound(Engine::GameObject::ObjectContext& a_context);

	private:

		// ---- 音(保存される) ----
		Engine::GUID m_soundGUID = Engine::DefaultGUID;
		Engine::Handle<Engine::Resource::SoundInstance> m_soundHandle = {};
		float m_volume = 1.0f;

		// ---- 表示(保存される) ----
		float m_showTime    = 0.15f;	// 1ヒットで出しておく時間(秒)
		float m_minInterval = 0.03f;	// 音を鳴らし直す最短間隔(秒)。
										// インスタンスは1つで Play は頭出しの鳴らし直しになるため、
										// 連射で潰れ続けないよう間引く
		bool  m_isFadeOut   = true;		// 消えるときにアルファを落とすか
		float m_punchScale  = 1.25f;	// 出た瞬間の拡大率(1.0 で拡縮なし)

		// ---- ランタイム ----
		float m_remainTime = 0.0f;		// 残り表示時間(秒)
		float m_coolTime   = 0.0f;		// 次に鳴らせるまでの残り時間(秒)
		int   m_hitCount   = 0;			// 出した回数(確認用)
	};
}
