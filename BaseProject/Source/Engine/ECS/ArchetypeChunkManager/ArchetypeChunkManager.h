#pragma once

#include "../Internal/EntityLocation.h"

struct ArchetypeChunk;

class ComponentMetaRegistry;

class ArchetypeChunkManager
{
public:

	ArchetypeChunkManager(ComponentMetaRegistry* a_pMetaRegister);
	~ArchetypeChunkManager();

	/// <summary>
	/// アーキタイプチャンクを取得
	/// </summary>
	/// <param name="a_sig">アーキタイプ指定</param>
	/// <returns>チャンクポインタ</returns>
	ArchetypeChunk* GetArchetypeChunk(const ECS::Signature& a_sig);

	/// <summary>
	/// エンティティを割り当てる
	/// </summary>
	/// <param name="a_entity">割り当てたいエンティティID</param>
	/// <param name="a_sig">割り当て先</param>
	/// <returns>割り当てた場所</returns>
	EntityLocation AllocateEntity(const ECS::Entity& a_entity,const ECS::Signature& a_sig);


	
	uint8_t* RefComponent(const EntityLocation& a_loca, const ECS::ComponentTypeID& a_typeID);

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
