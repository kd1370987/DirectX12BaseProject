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
	std::string 				baseColorTexKey;
	DirectX::XMFLOAT4			baseColor = { 1,1,1,1 };

	// メタリック・ラフネス
	std::string 				metallicRoughnessTexKey;
	float						metallic = 0.0f;						// B : 金属製
	float						roughness = 1.0f;						// G : 粗さ

	// エミッシブ
	std::string 				emissiveTexKey;
	DirectX::XMFLOAT3			emissive = { 1.0f,1.0f,1.0f };

	// 法線マップ
	std::string 				normalTexKey;

	DescriptorHandle			srvHandle;		// SRVハンドル
};