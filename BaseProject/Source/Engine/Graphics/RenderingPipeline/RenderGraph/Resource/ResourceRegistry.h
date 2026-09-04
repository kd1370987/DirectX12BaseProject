#pragma once

#include "VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	/// <summary>
	/// リソースレジストリー
	/// 
	/// 仮想リソースや外部リソースを登録、検索、取得、削除のみの管理クラス
	/// 仮想リソースはコンパイルのたびに組みなおす
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

		// 差し込まれている実体を名前で引く : 無ければ nullptr
		D3D12::GPUResource* FindImportedResource(const std::string& a_name) const;

		// 控えてある外部リソースを、仮想リソースとして先頭に並べる。
		// パスが知らないリソース(バックバッファなど)もここで席を持つ
		void SetupImportedResources();

		//----------------------------------------------------------------------------------
		// 仮想リソース
		//----------------------------------------------------------------------------------
		// 出力スロットの識別子で引いて、なければ新規生成。
		//
		// 描画解像度も渡す : スロットのサイズは「0 = 解像度に従う」なので、
		// 土台が無いと生成した時点で実サイズも占有サイズも出せない
		VirtualResource& Request(const Slot& a_outputSlot, UINT64 a_baseWidth, UINT a_baseHeight);

		// 識別子から仮想リソースを引く
		Index<VirtualResource> Find(ResourceID a_resourceID) const;

		// 識別子から仮想リソースの実体を引く : 無ければ nullptr
		const VirtualResource* GetByID(ResourceID a_resourceID) const;
		VirtualResource* RefByID(ResourceID a_resourceID);

		// 消去
		void Clear();

		// 仮想リソースだけを消去する : 外部リソースの控えは残す
		void ClearVirtualResources();

		// 参照
		const VirtualResource* Get(Index<VirtualResource> a_idx)const;
		VirtualResource* Ref(Index<VirtualResource> a_idx);

		const std::vector<VirtualResource>& GetVirtualResources() const { return m_virtualResourceVec; }
		std::vector<VirtualResource>& RefVirtualResources() { return m_virtualResourceVec; }

		uint32_t GetVirtualResourceCount() const { return static_cast<uint32_t>(m_virtualResourceVec.size()); }

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

		// リソースの対応表 : 鍵は識別子。
		// 名前で引いていたころは、別のパスが同じ名前を宣言すると黙って合流していた
		std::unordered_map<ResourceID, uint32_t> m_resourceIDMap = {};

		// 仮想リソース配列
		std::vector<VirtualResource> m_virtualResourceVec = {};

		// 外部から差し込まれたリソース
		std::vector<ImportedResource> m_importedResourceVec = {};
	};
}
