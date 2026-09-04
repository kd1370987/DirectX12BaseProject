#include "ResourceRegistry.h"
namespace Engine::Graphics::Pipeline
{
	void ResourceRegistry::ImportResource(
		const std::string& a_name,
		D3D12::GPUResource* a_pResource,
		D3D12_RESOURCE_STATES a_initialState,
		EPassSlotType a_type
	)
	{
		// 名前の重複チェック
		for (ImportedResource& _imported : m_importedResourceVec)
		{
			if (_imported.name != a_name) continue;

			// 同名であれば差し替える
			_imported.type = a_type;
			_imported.pResource = a_pResource;
			_imported.initialState = a_initialState;
			return;
		}

		// 新規なら配列に保存
		ImportedResource _imported = {};
		_imported.name = a_name;
		_imported.type = a_type;
		_imported.pResource = a_pResource;
		_imported.initialState = a_initialState;
		m_importedResourceVec.push_back(std::move(_imported));
	}
	void ResourceRegistry::RemoveImportedResource(const std::string& a_name)
	{
		// 配列から同名のものを探して、あれば消去する
		m_importedResourceVec.erase(
			std::remove_if(
				m_importedResourceVec.begin(),m_importedResourceVec.end(),
				[&a_name](const ImportedResource& a_imported)
				{
					return a_imported.name == a_name;
				}
			),
			m_importedResourceVec.end()
		);
	}
	void ResourceRegistry::ClearImportedResource()
	{
		m_importedResourceVec.clear();
	}
	D3D12::GPUResource* ResourceRegistry::FindImportedResource(const std::string& a_name) const
	{
		// 控えの配列から名前で探す
		for (const ImportedResource& _imported : m_importedResourceVec)
		{
			if (_imported.name != a_name) continue;
			return _imported.pResource;
		}
		return nullptr;
	}
	void ResourceRegistry::SetupImportedResources()
	{
		for (const ImportedResource& _imported : m_importedResourceVec)
		{
			if (_imported.name.empty()) continue;

			// 外部リソースの識別子は名前から起こす。
			// パスの出力ピン側も同じ名前から起こすので、ここで席を取っておけば合流する
			const ResourceID _resourceID = ResourceID::FromImportName(_imported.name);

			// 既に席があるものは飛ばす
			if (m_resourceIDMap.find(_resourceID) != m_resourceIDMap.end()) continue;

			// 外部で作られたリソースとして起こす
			VirtualResource _vRes = {};
			_vRes.SetupAsImported(_imported.name, _imported.type, _imported.initialState);

			// 配列に登録
			m_resourceIDMap[_resourceID] = static_cast<uint32_t>(m_virtualResourceVec.size());
			m_virtualResourceVec.push_back(std::move(_vRes));
		}
	}
	VirtualResource& ResourceRegistry::Request(const Slot& a_outputSlot, UINT64 a_baseWidth, UINT a_baseHeight)
	{
		// 識別子で検索
		auto _it = m_resourceIDMap.find(a_outputSlot.resourceID);
		if (_it != m_resourceIDMap.end())
		{
			// あれば返す
			return m_virtualResourceVec[_it->second];
		}

		// アウトプットスロットから仮想リソースを作成
		VirtualResource _vRes = {};
		_vRes.SetupFromOutputSlot(a_outputSlot, a_baseWidth, a_baseHeight);

		// 配列に登録
		m_resourceIDMap[a_outputSlot.resourceID] = static_cast<uint32_t>(m_virtualResourceVec.size());
		m_virtualResourceVec.push_back(std::move(_vRes));

		return m_virtualResourceVec.back();
	}
	Index<VirtualResource> ResourceRegistry::Find(ResourceID a_resourceID) const
	{
		if (!a_resourceID.IsValid()) return {};

		// 識別子で検索
		auto _it = m_resourceIDMap.find(a_resourceID);
		if (_it == m_resourceIDMap.end()) return {};

		// インデックスを生成して返す
		Index<VirtualResource> _idx(_it->second);
		return _idx;
	}
	const VirtualResource* ResourceRegistry::GetByID(ResourceID a_resourceID) const
	{
		return Get(Find(a_resourceID));
	}
	VirtualResource* ResourceRegistry::RefByID(ResourceID a_resourceID)
	{
		return Ref(Find(a_resourceID));
	}
	void ResourceRegistry::Clear()
	{
		ClearVirtualResources();

		ClearImportedResource();
	}
	void ResourceRegistry::ClearVirtualResources()
	{
		m_resourceIDMap.clear();
		m_virtualResourceVec.clear();
	}
	const VirtualResource* ResourceRegistry::Get(Index<VirtualResource> a_idx) const
	{
		if (!a_idx.IsValid() || a_idx.value >= static_cast<uint32_t>(m_virtualResourceVec.size())) 
			return nullptr;

		return &m_virtualResourceVec[a_idx.value];
	}
	VirtualResource* ResourceRegistry::Ref(Index<VirtualResource> a_idx)
	{
		if (!a_idx.IsValid() || a_idx.value >= static_cast<uint32_t>(m_virtualResourceVec.size()))
			return nullptr;

		return &m_virtualResourceVec[a_idx.value];
	}
}
