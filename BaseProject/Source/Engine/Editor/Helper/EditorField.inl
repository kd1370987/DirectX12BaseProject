#pragma once

//------------------------------------------------------------------------------------------
// EditorFieldのうち、ResourceManagerを必要とするテンプレート実装。
//
// EditorField.h本体は AnimatorAsset -> StateGraphEditor 経由で
// ResourceManagerより先に読まれるため、ResourceManagerを持ち込めない。
// そのため「ロードまで行うアセット欄」だけをこちらに分けている。
// AssetField<TResource> を使う側はこのファイルをインクルードする。
//------------------------------------------------------------------------------------------
#include "EditorField.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Editor
{
	template<typename TResource, typename THandle>
	bool AssetField(
		const ECS::EngineServices& a_services,
		const char* a_label,
		const char* a_assetTypeName,
		Engine::GUID& a_inoutGUID,
		THandle& a_inoutHandle
	)
	{
		GUID _selectedGUID = {};
		if (!AssetPicker(a_services, a_label, a_assetTypeName, a_inoutGUID, _selectedGUID))
		{
			return false;
		}

		if (!a_services.pResourceManager) return false;

		// ハンドルとGUIDを更新
		a_inoutHandle = a_services.pResourceManager->LoadImmediate<TResource>(_selectedGUID);
		a_inoutGUID = _selectedGUID;
		return true;
	}
}
