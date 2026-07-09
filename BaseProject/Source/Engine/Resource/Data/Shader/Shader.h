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
	};

	class Shader
	{
	public:
		Shader() = default;
		~Shader() = default;
		NON_COPYABLE_MOVABLE(Shader);
		// 読み込み
		void Load(
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

		// パス
		std::string m_path = "";

		// シェーダーデータ
		ComPtr<ID3DBlob> m_cpBlob;
		D3D12_SHADER_BYTECODE m_byteCode;

		EShaderStage m_stage = EShaderStage::Unknown;
	};
}