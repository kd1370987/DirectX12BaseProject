#pragma once

#include "../Internal/EntityLocation.h"

struct ArchetypeChunk;

class ComponentMetaRegistry;

class ArchetypeChunkManager
{
public:

	ArchetypeChunkManager();
	~ArchetypeChunkManager();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_pMetaRegister">コンポーネントメタレジストリ参照</param>
	void Init(ComponentMetaRegistry* a_pMetaRegister);

	/// <summary>
	/// アーキタイプチャンクを取得
	/// </summary>
	/// <param name="a_sig">アーキタイプ指定</param>
	/// <returns>チャンクポインタ</returns>
	const std::vector<ArchetypeChunk*>& GetArchetypeChunk(const ECS::Signature& a_sig);

	/// <summary>
	/// or検索でマッチするアーキタイプチャンク群を取得
	/// </summary>
	/// <param name="a_sig">絶対に入っていてほしいシグネチャ</param>
	/// <returns>ヒットしたアーキタイプポインタ配列</returns>
	std::vector<ArchetypeChunk*> MatchingArchetypeChunkVec(const ECS::Signature& a_sig);

	/// <summary>
	/// or,not検索でマッチするアーキタイプチャンク群を取得
	/// </summary>
	/// <param name="a_sig">入っていてほしいシグネチャ</param>
	/// <param name="a_excludeSig">含めないシグネチャ</param>
	/// <returns>ヒットしたアーキタイプポインタ配列</returns>
	std::vector<ArchetypeChunk*> MatchingArchetypeChunkVecEx(const ECS::Signature& a_sig, const ECS::Signature& a_excludeSig);

	/// <summary>
	/// エンティティを割り当てる
	/// </summary>
	/// <param name="a_entity">割り当てたいエンティティID</param>
	/// <param name="a_sig">割り当て先</param>
	/// <returns>割り当てた場所</returns>
	EntityLocation AllocateEntity(const ECS::Entity& a_entity,const ECS::Signature& a_sig);


	/// <summary>
	/// 単体にアクセス
	/// </summary>
	/// <param name="a_loca"></param>
	/// <param name="a_typeID"></param>
	/// <returns></returns>
	uint8_t* RefComponent(const EntityLocation& a_loca, const ECS::ComponentTypeID& a_typeID);

	uint8_t* RefComponentArray(ArchetypeChunk* a_chunk,const ECS::ComponentTypeID& a_typeID);

	//------------------------------------------------------------------------------------------
	// エンティティの削除
	//------------------------------------------------------------------------------------------

	/// <summary>
	/// エンティティの消去
	/// </summary>
	/// <param name="a_location">削除エンティティロケーション</param>
	/// <returns>スワップされたエンティティとインデックスを返す</returns>
	std::pair<ECS::Entity,uint32_t> RemoveEntity(const EntityLocation& a_location);


private:

	/// <summary>
	/// アーキタイプチャンクの生成
	/// </summary>
	/// <param name="a_sig">シグネチャ</param>
	/// <returns>生成したポインタ</returns>
	ArchetypeChunk* CreateArchetypeChunk(const ECS::Signature& a_sig);

	/// <summary>
	/// チャンク内のオフセットやメモリレイアウト計算
	/// </summary>
	/// <param name="a_pChunk">チャンクポインタ</param>
	/// <param name="a_sig">シグネチャ</param>
	/// <param name="a_memorySize">チャンクの使用メモリ上限</param>
	void CalcChunkLayout(ArchetypeChunk* a_pChunk,const ECS::Signature& a_sig, size_t a_memorySize);

private:

	ComponentMetaRegistry* m_pMetaRegister = nullptr;

	std::unordered_map<ECS::Signature, std::vector<ArchetypeChunk*>> m_pArchetypeChunkMap;

};
