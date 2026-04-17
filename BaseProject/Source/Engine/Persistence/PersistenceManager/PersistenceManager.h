#pragma once

namespace Engine::Persistence
{
	class PersistenceManager
	{
	public:
		// 捜査開始
		// 保存するファイルを置くディレクトリを指定
		void BegineObject(std::string a_path);

		// 指定されたディレクトリ内で保存する
		template<typename T>
		void Write(std::string a_key,T a_data);
		void Write(std::string a_key,uint8_t* a_pData);

		// 指定されたディレクトリ内で検索する
		template<typename T>
		T Read(std::string a_key);
		uint8_t* Read(std::string a_key);

		// 操作終了
		void EndObject();

	private:

		std::string m_currentDPath;

	};
}