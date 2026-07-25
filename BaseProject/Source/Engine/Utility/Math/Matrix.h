#pragma once
namespace Math::Matrix
{
	/// <summary>
	/// 返却用等の一時変数用
	/// </summary>
	struct TRS
	{
		DXSM::Vector3 pos;
		DXSM::Quaternion rotation;
		DXSM::Vector3 scale;
	};

	/// <summary>
	/// 対象の行列を分解する
	/// </summary>
	/// <param name="a_mat">行列</param>
	/// <returns>float3 pos, float4 rotation, float3 scaleの構造体に入れて返却</returns>
	inline TRS Decompose(const DirectX::XMFLOAT4X4& a_mat)
	{
		TRS _result = {};

		// 行列の分解
		DXSM::Matrix _mat(a_mat);
		_mat.Decompose(_result.scale, _result.rotation, _result.pos);

		return _result;
	}
}