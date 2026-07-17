#pragma once

namespace Engine::Resource
{
	class TextureIO
	{
	public:
		
		/// <summary>
		/// テクスチャの作成 : フライウェイトではないので呼び出し回数に注意
		/// </summary>
		/// <param name="a_initData">作成用構造体</param>
		/// <returns>リソースマネージャーに登録されたハンドル</returns>
		static Handle<Texture> Create(const TextureCreateDesc& a_initData);

		/// <summary>
		/// デフォルトロード
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>テクスチャの実体</returns>
		static Texture LoadFromFile(const std::string& a_path);

		/// <summary>
		/// デフォルトカラーを指定してテクスチャを読み込める
		/// </summary>
		/// <param name="a_guid">読み込みたいテクスチャGUID</param>
		/// <param name="a_defaultColor">読み込めなかったときのためのテクスチャカラー</param>
		/// <returns>リソースマネージャーに登録されたハンドル</returns>
		static Handle<Texture> LoadTexture(const Engine::GUID& a_guid,const DXSM::Color& a_defaultColor);

	private:

		// 色からGUIDを返す
		static Engine::GUID GetColorGUID(const DXSM::Color& a_color);

		// 単色テクスチャ作成 
		static Texture CreateColorTexture(const DXSM::Color& a_color);
	};
}