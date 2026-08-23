#pragma once
namespace Engine::Resource
{
	class FontIO
	{
	public:

		/// <summary>
		/// フォントファイル(.ttf / .otf / .ttc)を読み込む
		/// </summary>
		/// <param name="a_filePath">ファイルパス</param>
		/// <param name="a_pContext">
		/// ビルドコンテキスト。
		/// アトラステクスチャの初期転送をここのコマンドリストへ積むので、
		/// まとめて読むときは呼び出し元でバッチを開いて渡すこと
		/// </param>
		static Font LoadFromFile(const std::string& a_filePath, const ResourceBuildContext* a_pContext);
	};
}
