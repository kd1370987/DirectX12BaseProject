#pragma once

class Texture;

//==========================================================
// シェーダー描画用マテリアル
//==========================================================
struct Material
{
	//---------------------------------------
	// テクスチャのセット
	//---------------------------------------

	void SetTexture2D(
		const std::shared_ptr<Texture> a_spBaseColerTex,
		const std::shared_ptr<Texture> a_spMetallicRoughnessTex,
		const std::shared_ptr<Texture> a_spEmissiveTex,
		const std::shared_ptr<Texture> a_spNormalTex
	);

	void SetTexture2D(
		const std::string& a_fileDir,
		const std::string& a_baseColorTexFileName,
		const std::string& a_metallicRoughnessTexFileName,
		const std::string& a_emissiveTexFileName,
		const std::string& a_normalTexFileName
	);


	//---------------------------------------
	// 材質データ
	//---------------------------------------
	// 名前
	std::string					m_name;

	// 基本色
	std::shared_ptr<Texture>	m_spBaseColorTex = nullptr;
	DirectX::XMFLOAT4			m_baseColor = { 1,1,1,1 };

	// メタリック・ラフネス
	std::shared_ptr<Texture>	m_spMetallicRoughnessTex = nullptr;
	float						m_metallic = 0.0f;						// B : 金属製
	float						m_roughness = 1.0f;						// G : 粗さ

	// エミッシブ
	std::shared_ptr<Texture>	m_spEmissiveTex = nullptr;
	DirectX::XMFLOAT3			m_emissive = { 1.0f,1.0f,1.0f };

	// 法線マップ
	std::shared_ptr<Texture>	m_spNormalTex = nullptr;
};