#pragma once

namespace Engine::Resource
{
	class Shader
	{
	public:

		// 読み込み
		void Load(const std::string& a_path);

		// バイトコード取得
		D3D12_SHADER_BYTECODE GetByteCode();

	private:

		// シェーダーデータ
		ComPtr<ID3DBlob> m_cpBlob;
		D3D12_SHADER_BYTECODE m_byteCode;
	};
}