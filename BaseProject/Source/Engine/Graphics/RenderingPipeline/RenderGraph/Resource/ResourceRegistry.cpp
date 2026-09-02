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
			)
		);
	}
	void ResourceRegistry::ClearImportedResource()
	{
		m_importedResourceVec.clear();
	}
	VirtualResource& ResourceRegistry::Request(
		const std::string& a_name,
		const Slot& a_outputSlot
	)
	{
		// リソース名で検索
		auto _it = m_resourceNameMap.find(a_name);
		if (_it != m_resourceNameMap.end())
		{
			// あれば返す
			return m_virtualResourceVec[_it->second];
		}

		// アウトプットスロットから仮想リソースを作成
		VirtualResource _vRes = {};
		_vRes.SetupFromOutputSlot(a_name,a_outputSlot);

		// 配列に登録
		m_resourceNameMap[a_name] = static_cast<uint32_t>(m_virtualResourceVec.size());
		m_virtualResourceVec.push_back(std::move(_vRes));

		return m_virtualResourceVec.back();
	}
	Index<VirtualResource> ResourceRegistry::Find(const std::string& a_name) const
	{
		// リソース名で検索
		auto _it = m_resourceNameMap.find(a_name);
		if (_it == m_resourceNameMap.end()) return {};

		// インデックスを生成して返す
		Index<VirtualResource> _idx(_it->second);
		return _idx;
	}
	void ResourceRegistry::Clear()
	{
		m_resourceNameMap.clear();
		m_virtualResourceVec.clear();

		ClearImportedResource();
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