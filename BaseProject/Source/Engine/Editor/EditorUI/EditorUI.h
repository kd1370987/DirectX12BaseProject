#pragma once

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Editor::UI
{
	/// <summary>
	/// リソースマネージャーから特定の種類のアセットの検索欄を作成
	/// </summary>
	/// <typeparam name="TResource">リソースの型</typeparam>
	/// <typeparam name="THandle">リソースの型のハンドル</typeparam>
	/// <param name="a_lable">コンボボックスのラベル</param>
	/// <param name="a_assetTypeName">アセットデータベースに渡す型名</param>
	/// <param name="a_inoutGUID">上書きされるGUID</param>
	/// <param name="a_inoutHandle">上書きされるハンドル</param>
	/// <returns></returns>
	template<typename TResource,typename THandle>
	bool DrawAssetSelectCombo(
		const char* a_lable,
		const char* a_assetTypeName,
		Engine::GUID& a_inoutGUID,
		THandle& a_inoutHandle
	)
	{
		bool _isChanged = false;

		// 現在の選択情報
		auto* _pCurrentResource = Resource::ResourceManager::Instance().Ref(a_inoutHandle);
		if (_pCurrentResource)
		{
			ImGui::Text("%s : %s", a_assetTypeName, _pCurrentResource->GetName().c_str());
			ImGui::Text("%s", a_inoutGUID.String().c_str());
		}

		// 選択UI
		if (ImGui::BeginCombo(a_lable, "Select..."))
		{
			const auto& _assetList = Resource::AssetDatabase::Instance().GetTypeMetaVec(a_assetTypeName);
			for (const auto& _prop : _assetList)
			{
				bool _isSelected = (a_inoutGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _isSelected))
				{
					// ハンドルとGUIDを更新
					a_inoutHandle = Resource::ResourceManager::Instance().Load<TResource>(_prop.guid);
					a_inoutGUID = _prop.guid;
					_isChanged = true;
				}

				// コンボボックスを開いた際、現在の選択アイテムまで自動スクロールする
				if (_isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		return _isChanged;
	}

	/// <summary>
	/// エディター上でテクスチャを表示する
	/// </summary>
	/// <param name="a_handle">テクスチャのハンドル</param>
	/// <param name="a_width">横幅</param>
	/// <param name="a_height">縦</param>
	/// <returns>テクスチャのサイズ</returns>
	inline ImVec2 DrawTexture(
		const Handle<Resource::Texture>& a_handle,
		float a_width = 0,
		float a_height = 0
	)
	{
		// テクスチャ表示
		auto _pTex = Resource::ResourceManager::Instance().Ref(a_handle);
		if (!_pTex)
		{
			ImGui::Text("Not find texture");
			return { 0,0 };
		}
		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
		
		ImTextureID _imTex = (ImTextureID)(_gpuHandle.ptr);

		// 横幅だけを取得（縦の残り領域は無視する）
		float drawWidth = ImGui::GetContentRegionAvail().x;

		// アスペクト比を計算
		float aspect = a_width / a_height;

		// 横幅に合わせて高さを逆算する
		float drawHeight = drawWidth / aspect;

		// 計算したサイズで描画
		ImGui::Image(_imTex, ImVec2(drawWidth, drawHeight));

		// 実際に描画したサイズを返す
		return ImVec2(drawWidth, drawHeight);
	}
}