#pragma once
namespace Engine::Resource::Compiler
{
	/// <summary>
	/// シェーダーのコンパイル
	/// </summary>
	/// <param name="a_path">ファイルパス</param>
	/// <param name="a_args">コンパイル設定</param>
	/// <param name="a_targetProfile">生成時のシェーダーバージョン</param>
	/// <param name="a_entoryPointName">エントリーポイント名</param>
	/// <returns>ComPtrで包まれたバイトデータ</returns>
	ComPtr<IDxcBlob> ShaderCompile(
		const std::string& a_path, 
		std::vector<LPCWSTR>& a_args,
		const wchar_t* a_targetProfile,
		const wchar_t* a_entoryPointName
	);
}