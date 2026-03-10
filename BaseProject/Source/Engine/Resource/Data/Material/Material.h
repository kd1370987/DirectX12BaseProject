#pragma once

class Texture;

namespace Engine
{
	namespace Resource
	{
		enum class Alpha
		{
			Opaque,			// アルファ値を無視してすべてを不透明としてレンダリング
			Mask,			// アルファ値が閾値以下なら描画しない
			Blend			// アルファ値に基づいて背景色と描画する色を混ぜる
		};

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

			// アルファデータ
			Alpha alphaMode = Alpha::Opaque;

			// 基本色
			uint32_t					baseTexID = 0;
			DirectX::XMFLOAT4			baseColor = { 1,1,1,1 };
			Engine::Resource::Handle<Engine::Resource::Texture> baseColorTex = {};

			// メタリック・ラフネス
			uint32_t					metallicRoughnessTexID = 0;
			float						metallic = 0.0f;						// B : 金属製
			float						roughness = 1.0f;						// G : 粗さ
			Engine::Resource::Handle<Engine::Resource::Texture> metaRoughTex = {};

			// エミッシブ
			uint32_t					emissiveTexID = 0;
			DirectX::XMFLOAT3			emissive = { 1.0f,1.0f,1.0f };
			Engine::Resource::Handle<Engine::Resource::Texture> emissiveTex = {};

			// 法線マップ
			uint32_t					normalTexID = 0;
			Engine::Resource::Handle<Engine::Resource::Texture> normalTex = {};

			// SRVハンドル
			Engine::Resource::Handle<SRV> startSRVHandle;
		};
	}
}