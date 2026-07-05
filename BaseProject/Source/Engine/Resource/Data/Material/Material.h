#pragma once

namespace Engine::Resource
{
	enum class Alpha : uint8_t
	{
		Opaque = 0,			// アルファ値を無視してすべてを不透明としてレンダリング
		Mask = 1,			// アルファ値が閾値以下なら描画しない
		Blend = 2			// アルファ値に基づいて背景色と描画する色を混ぜる
	};

	//==========================================================
	// シェーダー描画用マテリアル
	//==========================================================
	struct Material
	{
		// 解放
		void Release();

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

		// セーブロード
		void Save(const std::string& a_fileDir, const std::string& a_name);
		void Load(const std::string& a_fileDir, const std::string& a_name);

		// ハッシュ値を返す
		UINT Hash();

		// 編集
		void Edit(const Engine::GUID& a_guid);

		//---------------------------------------
		// 材質データ
		//---------------------------------------
		// ---- シリアライズ用データ ----
		// 名前
		std::string					name;

		// アルファデータ
		Alpha alphaMode = Alpha::Opaque;

		// 参照シェーディングモデル
		Engine::GUID shedingModelGUID = {};
		Handle<ShadingModelTable> shadingModelHandle = {};

		// 参照テクスチャGUID
		Engine::GUID baseColorTexGUID = {};
		Engine::GUID metaRoughTexGUID = {};
		Engine::GUID emissiveTexGUID = {};
		Engine::GUID normalTexGUID = {};

		// 基本色
		DirectX::XMFLOAT4			baseColor = { 1,1,1,1 };
		Handle<Engine::Resource::Texture> baseColorTex = {};

		// メタリック・ラフネス
		float						metallic = 0.0f;						// B : 金属製
		float						roughness = 1.0f;						// G : 粗さ
		Handle<Engine::Resource::Texture> metaRoughTex = {};

		// エミッシブ
		DirectX::XMFLOAT3			emissive = { 1.0f,1.0f,1.0f };
		Handle<Engine::Resource::Texture> emissiveTex = {};

		// 法線マップ
		Handle<Engine::Resource::Texture> normalTex = {};
	};
}