#pragma once

namespace Engine::Resource
{
	/// <summary>
	/// シェーダーを要求してあれば読み込んで返して、
	/// なければコンパイルして保存されたものを生成して返す
	///
	/// インクルードパスには、指定されたシェーダー自身のディレクトリが必ず追加される。
	/// </summary>
	/// <param name="a_path">パス名</param>
	/// <param name="a_plofileVersion">シェーダーバージョン</param>
	/// <param name="a_setting">シェーダーの特殊設定 : なければデフォルト</param>
	/// <param name="a_forceStage">ステージの明示指定 : Unknown ならファイル名から推論</param>
	/// <returns>バイナリデータ</returns>
	ComPtr<ID3DBlob> RequestShader(
		const std::string& a_path,
		const wchar_t* a_plofileVersion = L"6_6",
		std::vector<LPCWSTR> a_setting = {},
		EShaderStage a_forceStage = EShaderStage::Unknown
	);

	namespace Import
	{

		// ブロッブ作成
		ComPtr<ID3DBlob> CompileShader(const std::string& a_path);

		// バイトコード作成
		D3D12_SHADER_BYTECODE CreateShaderByteCode(ID3DBlob* a_pBlob);

		// シェーダーステージの取得
		EShaderStage ReflectShaderStage(const std::string& a_path);
	}
}