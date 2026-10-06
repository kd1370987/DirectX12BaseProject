#pragma once
//==========================================================================================
//
// App 共通
//
// Core の名前を App の中から Core:: を付けずに使えるようにする(Engine/EngineCommon.h と同じ取り込み)。
// using namespace Core にしないのは、Core::GUID が Windows の ::GUID と曖昧になるため
//
//==========================================================================================
#include "Core/Core.h"

namespace App
{
	// 名前空間はそのまま別名で
	namespace Math = Core::Math;
	namespace String = Core::String;
	namespace File = Core::File;
	namespace TypeInfo = Core::TypeInfo;
	namespace Algorithm = Core::Algorithm;
	namespace BinaryHelper = Core::BinaryHelper;
	namespace Debug = Core::Debug;

	// 型・定数
	using Core::GUID;
	using Core::DEFAULT_GUID;

	// enum class のフラグ演算(演算子は名前で引けないと使えないため)
	using Core::operator|;
	using Core::operator|=;
	using Core::operator&;
	using Core::operator&=;
	using Core::operator~;
	using Core::HasFlag;
}
