#pragma once
namespace Engine::BinaryHelper
{
	// ---- 数値 ----
	// セーブ
	template<typename T>
	inline void Write(std::ofstream& a_ofs, const T& a_data)
	{
		a_ofs.write(reinterpret_cast<const char*>(&a_data),sizeof(T));
	}

	// ロード
	template<typename T>
	inline void Read(std::ifstream& a_ifs,T& a_outData)
	{
		a_ifs.read(reinterpret_cast<char*>(&a_outData),sizeof(T));
	}

	// ---- 文字 ----
	// セーブ
	inline void WriteString(std::ofstream& a_ofs, const std::string& a_str)
	{
		uint32_t _len = a_str.size();
		// 復元用にサイズを保存
		Write(a_ofs, _len);
		if (_len > 0)
		{
			// データを保存
			a_ofs.write(a_str.data(), _len);
		}
	}
	// ロード
	inline std::string ReadString(std::ifstream& a_ifs)
	{
		uint32_t _len = 0;
		// 文字列サイズを取得
		Read(a_ifs, _len);
		std::string _str(_len, '\0');
		if (_len > 0)
		{
			a_ifs.read(_str.data(), _len);
		}
		return _str;
	}

	// ---- 配列 ----
	// セーブ
	template<typename T>
	inline void WriteVector(std::ofstream& a_ofs, const std::vector<T>& a_vec)
	{
		uint32_t _size = a_vec.size();
		Write(a_ofs,_size);
		if (_size > 0)
		{
			a_ofs.write(reinterpret_cast<const char*>(a_vec.data()), sizeof(T) * _size);
		}
	}
	// ロード
	template<typename T>
	inline void ReadVector(std::ifstream& a_ifs, std::vector<T>& a_outVec)
	{
		uint32_t _size = 0;
		Read(a_ifs, _size);
		a_outVec.resize(_size);
		if (_size > 0)
		{
			a_ifs.read(reinterpret_cast<char*>(a_outVec.data()), sizeof(T) * _size);
		}
	}
}