#pragma once

#include "../Entity/EntityLocation.h"
#include "ChunkAllocator.h"

namespace Engine::ECS
{
	struct Archetype;
	struct Chunk;

	class ComponentMetaRegistry;

	//==========================================================================================
	// アーキタイプの管理
	//
	// アーキタイプ(コンポーネントの組み合わせ)ごとにレイアウトを1度だけ計算し、
	// そこへ属するチャンクを確保して並べる。
	//
	//   Archetype : シグネチャ・レイアウト・容量・チャンク一覧
	//   Chunk     : エンティティ配列とデータ本体(所属アーキタイプへの逆参照を持つ)
	//
	// チャンクは ChunkAllocator(World ごとに1つ)から借りて、空になったら返す。
	// 空いたチャンクは各アーキタイプで1つだけ手元に残し、2つ目が空いたら古い方を返す。
	//==========================================================================================
	class ArchetypeManager
	{
	public:

		ArchetypeManager();
		~ArchetypeManager();

		// コピー禁止(チャンクのメモリを所有しているため)
		ArchetypeManager(const ArchetypeManager&) = delete;
		ArchetypeManager& operator=(const ArchetypeManager&) = delete;

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="a_pMetaRegister">コンポーネントメタレジストリ参照</param>
		void Init(ComponentMetaRegistry* a_pMetaRegister);

		/// <summary>
		/// 世代取得 : チャンクが増減するたびに進む(クエリのキャッシュの作り直し判定に使う)
		/// </summary>
		uint64_t GetGeneration() const { return m_generation; }

		// 全アーキタイプ(生成順) : 参照のみ(プロファイラ用)
		const std::vector<std::unique_ptr<Archetype>>& GetArchetypeVec() const { return m_upArchetypeVec; }

		// チャンクの貸し出し元 : 参照のみ(プロファイラ用)
		const ChunkAllocator& GetChunkAllocator() const { return m_chunkAllocator; }

		// アーキタイプの取得 : 無ければ nullptr
		const Archetype* GetArchetype(const Signature& a_sig) const;

		// AND,NOT検索でマッチするアーキタイプ配列を取得
		std::vector<Archetype*> MatchingArchetypeVec(const Signature& a_sig, const Signature& a_excludeSig = {});

		// AND,NOT検索でマッチするアーキタイプに属するチャンクをすべて取得
		std::vector<Chunk*> MatchingChunkVec(const Signature& a_sig, const Signature& a_excludeSig = {});

		// エンティティを割り当てる : 割り当てられた場所を返す
		EntityLocation AllocationEntity(const Entity& a_entity, const Signature& a_sig);

		/// <summary>
		/// 単体にアクセス
		/// </summary>
		/// <param name="a_loca">エンティティの住所</param>
		/// <param name="a_typeID">コンポーネントのタイプID</param>
		/// <returns>持っていなければ nullptr</returns>
		uint8_t* RefComponent(const EntityLocation& a_loca, const ComponentTypeID& a_typeID);

		/// <summary>
		/// チャンク内のコンポーネント配列の先頭を取得
		/// </summary>
		/// <returns>持っていなければ nullptr</returns>
		uint8_t* RefComponentArray(Chunk* a_pChunk, const ComponentTypeID& a_typeID);

		/// <summary>
		/// エンティティの消去
		/// </summary>
		/// <param name="a_location">削除エンティティロケーション</param>
		/// <returns>スワップされたエンティティとインデックスを返す(誰も動かなければ INVALID_ENTITY)</returns>
		std::pair<Entity, uint32_t> RemoveEntity(const EntityLocation& a_location);

	private:

		// シグネチャに対応するアーキタイプを返す。無ければ作る
		Archetype* GetOrCreateArchetype(const Signature& a_sig);

		// アーキタイプの生成(レイアウト計算まで)
		Archetype* CreateArchetype(const Signature& a_sig);

		// アーキタイプにチャンクを1つ足す
		Chunk* CreateChunk(Archetype* a_pArchetype);

		// アーキタイプからチャンクを外してアロケーターへ返す
		void ReleaseChunk(Archetype* a_pArchetype, Chunk* a_pChunk);

		/// <summary>
		/// チャンク内のオフセットや容量の計算
		/// </summary>
		/// <param name="a_pArchetype">計算結果を書き込むアーキタイプ</param>
		/// <param name="a_memorySize">チャンクの使用メモリ上限</param>
		void CalcChunkLayout(Archetype* a_pArchetype, size_t a_memorySize);

		// すべてのチャンクをアロケーターへ返す
		void ReleaseAllChunks();

	private:

		// 1チャンクのデータ領域のサイズ
		static constexpr size_t CHUNK_MEMORY_SIZE = ChunkAllocator::CHUNK_MEMORY_SIZE;

		// 1ブロックで一括確保するチャンク数(64KB x 64 = 4MB)
		static constexpr size_t BLOCK_CHUNK_NUM = 64;

		ComponentMetaRegistry* m_pMetaRegister = nullptr;

		// チャンクの貸し出し元
		// ※ アーキタイプより先に宣言する(破棄はアーキタイプの後になる)
		ChunkAllocator m_chunkAllocator;

		// アーキタイプ配列(実体の持ち主)
		std::vector<std::unique_ptr<Archetype>> m_upArchetypeVec = {};

		// シグネチャからの引き当て(実体は m_upArchetypeVec)
		std::unordered_map<Signature, Archetype*> m_pArchetypeMap = {};

		// 構造変更(チャンクの追加・返却)が行われた回数
		uint64_t m_generation = 0;
	};
}
