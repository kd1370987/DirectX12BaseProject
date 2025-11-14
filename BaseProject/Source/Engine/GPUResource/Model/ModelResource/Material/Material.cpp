#include "Material.h"

void Material::SetTexture2D(
	const std::shared_ptr<Texture2D> a_spBaseColerTex,
	const std::shared_ptr<Texture2D> a_spMetallicRoughnessTex, 
	const std::shared_ptr<Texture2D> a_spEmissiveTex, 
	const std::shared_ptr<Texture2D> a_spNormalTex
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
	std::shared_ptr<Texture2D> _baseColorTex = nullptr;
	std::shared_ptr<Texture2D> _metallicRoughnessTex = nullptr;
	std::shared_ptr<Texture2D> _emissiveTex = nullptr;
	std::shared_ptr<Texture2D> _normalTex = nullptr;

	if (!a_baseColorTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_baseColorTexFileName))
	{
		_baseColorTex = std::make_shared<Texture2D>();
	}
}
