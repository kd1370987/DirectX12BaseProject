#pragma once

#include "Vector/Vector3.h"
#include "Quaternion.h"
#include "Matrix.h"

//==========================================================================================
// Math::TRS
//
// 行列を分解した結果をまとめて受け取るための入れ物。
// 実体を持つので、構造体の定義が全部そろっているここに置く
// (Vector3 / Quaternion / Matrix の各ヘッダは互いに前方宣言だけで完結させたいため、
//  「3つとも必要」なものはこちらへ分けている)。
//==========================================================================================
namespace Math
{
	struct TRS
	{
		Vector3    pos;
		Quaternion rotation;
		Vector3    scale;
	};

	/// <summary>行列を分解して TRS で受け取る</summary>
	TRS Decompose(const Matrix& a_matrix) noexcept;
}
