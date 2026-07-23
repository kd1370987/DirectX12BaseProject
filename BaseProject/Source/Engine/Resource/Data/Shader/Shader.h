#pragma once

namespace Engine::Resource
{
	enum class EShaderStage
	{
		Unknown,
		VS,
		PS,
		GS,
		HS,
		DS,
		CS,
		MS,
		AS,
		Lib,	// DXILライブラリ(レイトレ用)
	};

	//==========================================================================================
	//
	// シェーダー
	//
	// ラスタライザ用もレイトレ用(DXILライブラリ)も、このクラス1つで扱う。
	// どちらも DXC で同じ手順でコンパイルするため分ける理由がなく、
	// 統一することで .cso キャッシュとエラー出力がレイトレ側にも効く。
	//
	//==========================================================================================
	class Shader
	{
	public:
		Shader() = default;
		~Shader() = default;
		NON_COPYABLE_MOVABLE(Shader);

		// 読み込み
		// プロファイルとエントリはファイル名のサフィックス(VS/PS/CS/MS/AS/LB)から推論する
		void Load(
			const std::string& a_path,
			std::vector<LPCWSTR> a_setting = {},
			const wchar_t* a_version = L"6_6"
		);

		// DXILライブラリとして読み込む
		// ファイル名から推論できないレイトレ用シェーダーはこちらを使う
		void LoadLibrary(
			const std::string& a_path,
			std::vector<LPCWSTR> a_setting = {},
			const wchar_t* a_version = L"6_6"
		);

		// 解放
		void Release();

		// バイトコード取得
		const D3D12_SHADER_BYTECODE& GetByteCode() const;

		ID3DBlob* Get();
		EShaderStage GetStage() const { return m_stage; }
	private:

		// 読み込み本体
		void LoadInternal(
			const std::string& a_path,
			std::vector<LPCWSTR> a_setting,
			const wchar_t* a_version,
			EShaderStage a_forceStage
		);

	private:

		// パス
		std::string m_path = "";

		// シェーダーデータ
		ComPtr<ID3DBlob> m_cpBlob;
		D3D12_SHADER_BYTECODE m_byteCode;

		EShaderStage m_stage = EShaderStage::Unknown;
	};
}
