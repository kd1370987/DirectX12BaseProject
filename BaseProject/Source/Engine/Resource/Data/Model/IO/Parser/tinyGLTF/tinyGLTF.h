#pragma once

#include "../ParserStruct.h"

namespace Engine::Resource
{
	namespace GLTF
	{
		//=========================================================
		// GLTF/GLBのパース
		//
		// ここでは「ファイルに書かれている情報をそのまま」中間素材へ移すだけ。
		// 座標系の変換(Zミラーなど)やスケール調整は ModelProcessor の担当なので
		// この層では一切行わないこと。
		//=========================================================

		/// <summary>
		/// GLTFモデルの読み込み
		/// </summary>
		/// <param name="a_filePath">モデルファイルパス</param>
		/// <returns>読み込んだままの中間素材</returns>
		Parse::RawModel Load(std::string_view a_filePath);
	}
}
