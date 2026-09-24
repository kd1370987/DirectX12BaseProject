#pragma once

#include "EntityManager.h"
#include "../Archetype/ArchetypeManager.h"
#include "CommandBuffer.h"

namespace Engine::ECS
{
	class ComponentMetaRegistry;
	struct EngineServices;

	//==========================================================================================
	// エンティティの置き場
	//
	// エンティティID(EntityManager)とチャンク上の実体(ArchetypeManager)を
	// 食い違わないように一緒に動かす。生成・削除・引っ越しの実処理はすべてここ。
	//
	// 「いつ動かすか」(予約と反映の順番)や「初期化をやり直すか」といった
	// 決めごとは World が持ち、ここは言われたとおりに動かすだけ。
	// 反復(ForEach / システム)の最中に構造を変える関数を呼んではいけない。
	//==========================================================================================
	class EntityStorage
	{
	public:

		// 初期化 : 型情報と、解放フックに渡すサービスを受け取る
		void Init(ComponentMetaRegistry* a_pRegistry, const EngineServices* a_pServices);

		//------------------------------------------------------------------------------------------
		// 構造の変更
		//------------------------------------------------------------------------------------------

		/// <summary>
		/// エンティティを作り、全コンポーネントを既定値で構築する
		/// </summary>
		/// <returns>作れなければ INVALID_ENTITY</returns>
		Entity Create(const Signature& a_sig);

		/// <summary>
		/// エンティティを消す : 借りているものを返してからチャンクと ID を解放する
		/// </summary>
		void Destroy(const Entity& a_entity);

		/// <summary>
		/// エンティティを別のアーキタイプへ引っ越す
		/// </summary>
		/// <param name="a_toSig">引っ越し先のシグネチャ</param>
		/// <param name="a_initData">引っ越し先で上書きする初期値</param>
		/// <param name="a_isReleaseAll">持っているもの全部を返させるか(初期化をやり直すとき)</param>
		void Move(const Entity& a_entity, const Signature& a_toSig, const ComponentDataMap& a_initData, bool a_isReleaseAll);

		/// <summary>
		/// 初期値のバイト列をコンポーネントへ書き込む(持っていないものは飛ばす)
		/// </summary>
		void WriteComponentData(const Entity& a_entity, const ComponentDataMap& a_dataMap);

		//------------------------------------------------------------------------------------------
		// 参照
		//------------------------------------------------------------------------------------------

		bool IsAlive(const Entity& a_entity) { return m_entityManager.IsAlive(a_entity); }
		const Signature& GetSignature(const Entity& a_entity) { return m_entityManager.GetSignature(a_entity); }
		const EntityLocation& GetLocation(const Entity& a_entity) { return m_entityManager.GetLocation(a_entity); }
		const std::vector<EntityLocation>& GetAllEntityLocation() { return m_entityManager.GetAllEntityLocation(); }
		UINT GetAliveEntityCount() { return m_entityManager.GetAliveEntityCount(); }

		// コンポーネント単体 : 持っていなければ nullptr
		uint8_t* RefComponent(const Entity& a_entity, ComponentTypeID a_typeID);

		// チャンク内のコンポーネント配列の先頭 : 持っていなければ nullptr
		uint8_t* RefComponentArray(Chunk* a_pChunk, ComponentTypeID a_typeID) { return m_archetypeManager.RefComponentArray(a_pChunk, a_typeID); }

		// AND,NOT 検索でマッチするチャンクをすべて取得
		std::vector<Chunk*> MatchingChunkVec(const Signature& a_sig, const Signature& a_excludeSig = {}) { return m_archetypeManager.MatchingChunkVec(a_sig, a_excludeSig); }

		// アーキタイプの世代 : チャンクが増減するたびに進む
		uint64_t GetArchetypeGeneration() const { return m_archetypeManager.GetGeneration(); }

		// 中身の参照のみ(プロファイラ用)
		const EntityManager& GetEntityManager() const { return m_entityManager; }
		const ArchetypeManager& GetArchetypeManager() const { return m_archetypeManager; }

	private:

		// チャンクへ載せて、住所とシグネチャを記録する
		void AttachToChunk(const Entity& a_entity, const Signature& a_sig);

		// チャンクから抜く。末尾を穴へ詰めたときは、動いたエンティティの住所を直す
		void DetachFromChunk(const EntityLocation& a_location);

		// 持っているコンポーネントを値として退避する(引っ越しで実体の置き場所が変わるため)
		ComponentDataMap SnapshotComponents(const Entity& a_entity, const Signature& a_sig);

		// 退避したデータのうち、引き継がないものに借りているものを返させる
		void ReleaseBeforeMove(ComponentDataMap& a_snapshot, const Signature& a_toSig, const ComponentDataMap& a_initData, bool a_isReleaseAll);

		// 引っ越し先にも残るコンポーネントへ、退避したデータを書き戻す
		void RestoreComponents(const Entity& a_entity, const Signature& a_toSig, const ComponentDataMap& a_snapshot);

		// コンポーネントが借りているものを返させる(実体 / 退避したバッファ)
		void ReleaseComponents(const Entity& a_entity, const Signature& a_sig);
		void ReleaseComponentData(ComponentTypeID a_compID, uint8_t* a_pData);

	private:

		EntityManager		m_entityManager;
		ArchetypeManager	m_archetypeManager;

		ComponentMetaRegistry*	m_pRegistry = nullptr;
		const EngineServices*	m_pServices = nullptr;	// 解放フックに渡す
	};
}
