#pragma once

#include "../Internal/SystemComon.h"

#include "../System/ISystem/ISystem.h"

class World;

class SystemManager
{
public:

	template<typename T>
	void Register()
	{
		m_upSystemVec.push_back(std::make_unique<T>());
	}

	void Register(
		std::unique_ptr<ISystem> a_upSystem
	);

	void RunSystem(
		World& a_world,const SystemType& a_type, float a_dt
	);

	void Sort();	// 未実装

private:

	// 一時記憶＆整理用
	std::vector<std::unique_ptr<ISystem>> m_upSystemVec;

	// 実際に実行されるシステム配列グループ
	std::vector<std::vector<ISystem*>> m_pSystemGroupVec;

};