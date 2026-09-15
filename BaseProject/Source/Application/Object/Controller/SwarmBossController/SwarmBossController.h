#pragma once

#include "Engine/GameObject/BaseObject/BaseObject.h"


namespace App::Object
{
	/// <summary>
	/// 砂漠ステージのワームボスの管理用クラス
	/// 
	/// リーダー、小隊長を動かして制御するがボイドは各自に任せる
	/// </summary>
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

	private:
		//------------------------------------------------------------------------------------------
		// 生成
		//------------------------------------------------------------------------------------------
		bool CreateLeader(Engine::GameObject::ObjectContext& a_context);
		bool CreatePlatoonLeaders(Engine::GameObject::ObjectContext& a_context);
	private:
		// 生成されたかどうか
		bool m_isSpown = false;

		//------------------------------------------------------------------------------------------
		// ボスの構成要素
		//------------------------------------------------------------------------------------------
		// 先頭のリーダ : このクラスから指示を出す対象
		Engine::ResourceRef<Engine::Resource::Prefab> m_leaderPrefabHandle;
		Engine::ECS::Entity m_leaderEntity = Engine::ECS::Limits::INVALID_ENTITY;

		// 構成する小隊長 : 基本的にリーダーに追従する処理はECS側
		Engine::ResourceRef<Engine::Resource::Prefab> m_platoonPrefab;	// 小隊長のプレハブ
		std::vector<Engine::ECS::Entity> m_platoonLeaderEntities = {};	// 生存している小隊長
		uint32_t m_maxPlatoonLeader = 0;								// 最大小隊長数

		// 自身の体を構成しているボイド数 : タグをつけて収集 操作などはしない
		uint32_t m_currentBoids = 0;									// 残りの生存数 : HP代わり
		uint32_t m_maxBoid = 0;											// 最大生成数 : 小隊長の数で割って振り分ける

		//------------------------------------------------------------------------------------------
		// ボスの行動データ
		//------------------------------------------------------------------------------------------
		Math::Vector3 m_targetPos = {};		// テスト用 : リーダーのターゲット位置
	};
}
