#pragma once

namespace Engine::Persistence
{
	class SerializerBase
	{
	public:

		// 保存時フォーマット指定
		// 配列
		virtual void BeginArray(const std::string&) = 0;
		virtual void EndArray() = 0;
		// 単体
		virtual void BegineObject(const std::string&) = 0;
		virtual void EndObject() = 0;

		// 保存
		virtual void WriteInt(const std::string&, int) = 0;
		virtual void ReadInt(const std::string&) = 0;


	protected:

	};
}