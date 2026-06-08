#pragma once

namespace Engine::Resource
{
	class ShaderLibrary
	{
	public:

		// 読み込み
		void Load(const std::string& a_path);

		// 解放
		void Release();

		// バイトデーター取得
		IDxcBlob* GetIDxcBlob();

	private:

		ComPtr<IDxcBlob> m_cpIDxcBlob;
	};
}