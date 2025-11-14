#include "Material.h"

#include "Engine/GPUResource/Texture/Texture.h"

#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/GPUResource/DescriptorHeap/DescriptorHeap.h"

void Material::SetTexture2D(
	const std::shared_ptr<Texture> a_spBaseColerTex,
	const std::shared_ptr<Texture> a_spMetallicRoughnessTex, 
	const std::shared_ptr<Texture> a_spEmissiveTex, 
	const std::shared_ptr<Texture> a_spNormalTex
)
{
	m_spBaseColorTex = a_spBaseColerTex;
	m_spMetallicRoughnessTex = a_spMetallicRoughnessTex;
	m_spEmissiveTex = a_spEmissiveTex;
	m_spNormalTex = a_spNormalTex;

	// メタリック・ラフネステクスチャがある場合は、デフォルト値を変更
	if (a_spMetallicRoughnessTex)
	{
		m_metallic = 1.0f;
		m_roughness = 1.0f;
	}
}

void Material::SetTexture2D(
	const std::string& a_fileDir, 
	const std::string& a_baseColorTexFileName, 
	const std::string& a_metallicRoughnessTexFileName, 
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	// テクスチャ準備
	std::shared_ptr<Texture> _baseColorTex = nullptr;
	std::shared_ptr<Texture> _metallicRoughnessTex = nullptr;
	std::shared_ptr<Texture> _emissiveTex = nullptr;
	std::shared_ptr<Texture> _normalTex = nullptr;

	// 基本色テクスチャ
	if (!a_baseColorTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_baseColorTexFileName))
	{
		_baseColorTex = std::make_shared<Texture>();
		if (_baseColorTex->Load(a_fileDir + a_baseColorTexFileName))
		{
			// 読み込み成功
			// ディスクリプタヒープに登録
			DescriptorHeapManager::Instance().RegisterSRV(_baseColorTex->GetResource());
		}
		else
		{
			_baseColorTex = nullptr;
		}
	}
	// メタリック・ラフネステクスチャ
	if (!a_metallicRoughnessTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_metallicRoughnessTexFileName))
	{
		_metallicRoughnessTex = std::make_shared<Texture>();
		_metallicRoughnessTex->Load(a_fileDir + a_metallicRoughnessTexFileName);

		// ディスクリプタヒープに登録
		DescriptorHeapManager::Instance().RegisterSRV(_metallicRoughnessTex->GetResource());
	}
	// エミッシブテクスチャ
	if (!a_emissiveTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_emissiveTexFileName))
	{
		_emissiveTex = std::make_shared<Texture>();
		_emissiveTex->Load(a_fileDir + a_emissiveTexFileName);

		// ディスクリプタヒープに登録
		DescriptorHeapManager::Instance().RegisterSRV(_emissiveTex->GetResource());
	}
	// 法線マップテクスチャ
	if (!a_normalTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_normalTexFileName))
	{
		_normalTex = std::make_shared<Texture>();
		_normalTex->Load(a_fileDir + a_normalTexFileName);

		// ディスクリプタヒープに登録
		DescriptorHeapManager::Instance().RegisterSRV(_normalTex->GetResource());
	}

	SetTexture2D(_baseColorTex, _metallicRoughnessTex, _emissiveTex, _normalTex);
}
