#pragma once

#include "VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	/// <summary>
	/// リソースレジストリー
	/// 
	/// 仮想リソースや外部リソースを登録、検索、取得、削除のみの管理クラス
	/// </summary>
	class ResourceRegistry
	{
	public:

		//----------------------------------------------------------------------------------
		// 外部リソース
		//----------------------------------------------------------------------------------
		// グラフ外リソースを名前で差し込む : バッファ関連
		void ImportResource(
			const std::string& a_name,
			D3D12::GPUResource* a_pResource,
			D3D12_RESOURCE_STATES a_initialState,
			EPassSlotType a_type
		);

		// 差し込んでいた外部リソースの登録を外す
		void RemoveImportedResource(const std::string& a_name);

		// 外部リソースの配列を全消去
		void ClearImportedResource();

		//----------------------------------------------------------------------------------
		// 仮想リソース
		//----------------------------------------------------------------------------------
		// 名前があればそれを返して、なければ新規生成
		VirtualResource& Request(const std::string& a_name,const Slot& a_outputSlot);

		// 名前から仮想リソースを引く
		Index<VirtualResource> Find(const std::string& a_name) const;

		// 消去
		void Clear();

		// 参照
		const VirtualResource* Get(Index<VirtualResource> a_idx)const;
		VirtualResource* Ref(Index<VirtualResource> a_idx);

		const std::vector<VirtualResource>& GetVirtualResources() const { return m_virtualResourceVec; }

	private:

		// 外部から差し込まれたリソースの控え。
		// Compile() が仮想リソース配列を作り直すので、元の情報はこちらに残しておく
		struct ImportedResource
		{
			std::string name = "";
			EPassSlotType type = EPassSlotType::Texture;
			D3D12::GPUResource* pResource = nullptr;
			D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
		};

		// リソースの名前対応表
		std::unordered_map<std::string, uint32_t> m_resourceNameMap = {};

		// 仮想リソース配列
		std::vector<VirtualResource> m_virtualResourceVec = {};

		// 外部から差し込まれたリソース
		std::vector<ImportedResource> m_importedResourceVec = {};
	};
}