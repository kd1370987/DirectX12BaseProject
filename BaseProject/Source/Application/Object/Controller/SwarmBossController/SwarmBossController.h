#pragma once

#include "Engine/GameObject/BaseObject/BaseObject.h"


namespace App::Object
{
	/// <summary>
	/// 砂漠ステージのワームボスの管理用クラス
	///
	/// リーダー、小隊長を動かして制御するがボイドは各自に任せる
	/// </summary>
	/// <remarks>
	/// 生成の流れ : オブジェクト生成 → リーダー → 小隊長(最大数ぶん) → 各小隊長のボイド
	///
	/// ・リーダーと小隊長はエンティティIDを握りたいので即時生成で出す
	///   (小隊長に「一つ前の相手」を覚えさせるため。遅延生成だとIDが返らない)。
	///   Awake は ECS の反復の外で呼ばれるので、その場で CreateEntity してよい。
	/// ・移動に要るコンポーネントと役割の印は、プレハブに無ければ生成時に足す。
	/// ・ボイドには SpawnerComponent(このオブジェクトのGUID + 小隊番号)を付ける。
	///   生存数はその印を数えるだけで、ボイドのIDは持ち歩かない。
	/// ・出した一式はシーンに保存される(GUIDComponent を持つため)。
	///   生成後に保存したシーンを読み直すと一式が重なるので、保存は生成前の状態で行うこと。
	/// </remarks>
	class SwarmBossController : public Engine::GameObject::BaseObject
	{
	public:
		//------------------------------------------------------------------------------------------
		// 初期化
		//------------------------------------------------------------------------------------------
		void PostDeserialize(Engine::GameObject::ObjectContext& a_context) override;
		void Awake(Engine::GameObject::ObjectContext& a_context) override;
		void Start(Engine::GameObject::ObjectContext& a_context) override;

		//------------------------------------------------------------------------------------------
		// 更新
		//------------------------------------------------------------------------------------------
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		//------------------------------------------------------------------------------------------
		// シリアライズ / エディター
		//------------------------------------------------------------------------------------------
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		const char* GetEditorName() const override { return "SwarmBossController"; }
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:
		//------------------------------------------------------------------------------------------
		// 生成
		//------------------------------------------------------------------------------------------
		// リーダー → 小隊長 → ボイドの順にまとめて出す。出し済みなら何もしない
		bool Spawn(Engine::GameObject::ObjectContext& a_context);

		bool CreateLeader(Engine::GameObject::ObjectContext& a_context);
		bool CreatePlatoonLeaders(Engine::GameObject::ObjectContext& a_context);

		// 小隊長1体ぶんのボイドを、その小隊長の BoidSpownerComponent の設定で出す
		bool CreateBoids(
			Engine::GameObject::ObjectContext& a_context,
			Engine::ECS::Entity a_platoonLeader,
			int a_platoonIndex,
			uint32_t a_count,
			const Math::Vector3& a_center);

		// 小隊長 a_platoonIndex 番に振り分けるボイド数(最大数を小隊長の数で割り、余りは前から1体ずつ)
		uint32_t GetBoidCountForPlatoon(size_t a_platoonIndex, size_t a_platoonCount) const;

		// 自分の印が付いた生存ボイドを数える
		uint32_t CountAliveBoids(Engine::GameObject::ObjectContext& a_context) const;

	private:
		// 生成されたかどうか
		bool m_isSpown = false;

		//------------------------------------------------------------------------------------------
		// ボスの構成要素
		//------------------------------------------------------------------------------------------
		// 先頭のリーダ : このクラスから指示を出す対象
		Engine::GUID m_leaderPrefabGUID = {};								// 保存用
		Engine::ResourceRef<Engine::Resource::Prefab> m_leaderPrefabHandle;	// ランタイム用
		Engine::ECS::Entity m_leaderEntity = Engine::ECS::Limits::INVALID_ENTITY;
		Math::Vector3 m_spawnPos = {};										// リーダーの生成位置(ワールド)

		// 構成する小隊長 : 基本的にリーダーに追従する処理はECS側
		// 並びはリーダーの後ろ(-Z)へ一列。間隔は小隊長プレハブの PlatoonLeaderComponent.distance
		Engine::GUID m_platoonPrefabGUID = {};							// 小隊長のプレハブ(保存用)
		Engine::ResourceRef<Engine::Resource::Prefab> m_platoonPrefab;	// 小隊長のプレハブ(ランタイム用)
		std::vector<Engine::ECS::Entity> m_platoonLeaderEntities = {};	// 生存している小隊長
		uint32_t m_maxPlatoonLeader = 0;								// 最大小隊長数

		// 自身の体を構成しているボイド数 : タグをつけて収集 操作などはしない
		// 出すボイドのプレハブと広さは小隊長プレハブの BoidSpownerComponent が持つ
		uint32_t m_currentBoids = 0;									// 残りの生存数 : HP代わり
		uint32_t m_maxBoid = 0;											// 最大生成数 : 小隊長の数で割って振り分ける

		//------------------------------------------------------------------------------------------
		// ボスの行動データ
		//------------------------------------------------------------------------------------------
		Math::Vector3 m_targetPos = {};		// テスト用 : リーダーのターゲット位置
	};
}
