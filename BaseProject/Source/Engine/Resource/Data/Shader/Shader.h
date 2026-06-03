#pragma once

namespace Engine::Resource
{
	class Shader
	{
	public:

		// 読み込み
		void Load(const std::string& a_path);

		// バイトコード取得
		const D3D12_SHADER_BYTECODE& GetByteCode() const;
		
		ID3DBlob* Get();

	private:

		// パス
		std::string m_path = "";

		// シェーダーデータ
		ComPtr<ID3DBlob> m_cpBlob;
		D3D12_SHADER_BYTECODE m_byteCode;
	};
}