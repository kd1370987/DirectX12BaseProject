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
	std::string					name;

	// 基本色
	std::shared_ptr<Texture>	spBaseColorTex = nullptr;
	//std::string 				baseColorTexKey;
	DirectX::XMFLOAT4			baseColor = { 1,1,1,1 };

	// メタリック・ラフネス
	std::shared_ptr<Texture>	spMetallicRoughnessTex = nullptr;
	//std::string 				metallicRoughnessTexKey;
	float						metallic = 0.0f;						// B : 金属製
	float						roughness = 1.0f;						// G : 粗さ

	// エミッシブ
	std::shared_ptr<Texture>	spEmissiveTex = nullptr;
	//std::string 				emissiveTexKey;
	DirectX::XMFLOAT3			emissive = { 1.0f,1.0f,1.0f };

	// 法線マップ
	std::shared_ptr<Texture>	spNormalTex = nullptr;
	//std::string 				normalTexKey;
};