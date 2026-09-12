#pragma once

// DXC はシェーダーのコンパイル経路だけで使う。
// プリコンパイル済みヘッダーへ置くと全翻訳単位に広がるため
#pragma warning(push, 0)
#include <dxcapi.h>
#pragma warning(pop)

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