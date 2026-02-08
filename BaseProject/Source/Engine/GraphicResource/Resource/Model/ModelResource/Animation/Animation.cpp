#include "Animation.h"

void Animation::Interpolate(AnimationNode& a_node,DirectX::XMFLOAT4X4& a_rDst)
{
	bool _isChange = false;

	DXSM::Vector3 _rs = {};
	DXSM::Quaternion _rq = {};
	DXSM::Vector3 _rt = {};
	if (InterpolateScale(a_node,_rs)) _isChange = true;
	if (InterpolateRotations(a_node, _rq)) _isChange = true;
	if (InterpolateTranslations(a_node, _rt)) _isChange = true;

	if (_isChange)
	{
		DXSM::Matrix _sMat = DXSM::Matrix::CreateScale(_rs);
		DXSM::Matrix _rMat = DXSM::Matrix::CreateFromQuaternion(_rq);
		DXSM::Matrix _tMat = DXSM::Matrix::CreateTranslation(_rt);
		a_rDst = _sMat * _rMat * _tMat;
	}
}

bool Animation::InterpolateTranslations(AnimationNode& a_node, DXSM::Vector3& a_resullt)
{
	if (a_node.translations.size() == 0) return false;

	// キー検索
	UINT _keyIdx = BinarySearchNextAnimKey(a_node.translations);

	// 先頭のおキーなら、先頭のデータを返す
	if (_keyIdx == 0)
	{
		a_resullt = a_node.translations.front().vec;
		return true;
	}
	// 配列外のキーなら最後のデータを返す
	else if (_keyIdx >= a_node.translations.size())
	{
		a_resullt = a_node.translations.back().vec;
		return true;
	}
	else
	{
		auto& _prev = a_node.translations[_keyIdx - 1];		// 前のキー
		auto& _next = a_node.translations[_keyIdx];		// 次のキー

		// 前のキーと次のキーの時間から、0~1間の時間を求める
		float _f = (1.0f - _prev.time) / (_next.time - _prev.time);

		// 補間
		a_resullt = DirectX::XMVectorLerp(
			DirectX::XMLoadFloat3(&_prev.vec),
			DirectX::XMLoadFloat3(&_next.vec),
			_f
		);
	}

    return true;
}

bool Animation::InterpolateRotations(AnimationNode& a_node, DXSM::Quaternion& a_resullt)
{
	if (a_node.rotations.size() == 0) return false;

	// キー検索
	UINT _keyIdx = BinarySearchNextAnimKey(a_node.rotations);

	// 先頭のおキーなら、先頭のデータを返す
	if (_keyIdx == 0)
	{
		a_resullt = a_node.rotations.front().quat;
		return true;
	}
	// 配列外のキーなら最後のデータを返す
	else if (_keyIdx >= a_node.rotations.size())
	{
		a_resullt = a_node.rotations.back().quat;
		return true;
	}
	else
	{
		auto& _prev = a_node.rotations[_keyIdx - 1];		// 前のキー
		auto& _next = a_node.rotations[_keyIdx];		// 次のキー

		// 前のキーと次のキーの時間から、0~1間の時間を求める
		float _f = (1.0f - _prev.time) / (_next.time - _prev.time);

		// 補間
		a_resullt = DirectX::XMQuaternionSlerp(
			DirectX::XMLoadFloat4(&_prev.quat),
			DirectX::XMLoadFloat4(&_next.quat),
			_f
		);
	}

	return true;
}

bool Animation::InterpolateScale(AnimationNode& a_node, DXSM::Vector3& a_resullt)
{
	if (a_node.scales.size() == 0) return false;

	// キー検索
	UINT _keyIdx = BinarySearchNextAnimKey(a_node.scales);

	// 先頭のおキーなら、先頭のデータを返す
	if (_keyIdx == 0)
	{
		a_resullt = a_node.scales.front().vec;
		return true;
	}
	// 配列外のキーなら最後のデータを返す
	else if (_keyIdx >= a_node.scales.size())
	{
		a_resullt = a_node.scales.back().vec;
		return true;
	}
	else
	{
		auto& _prev = a_node.scales[_keyIdx - 1];		// 前のキー
		auto& _next = a_node.scales[_keyIdx];		// 次のキー

		// 前のキーと次のキーの時間から、0~1間の時間を求める
		float _f = (1.0f - _prev.time) / (_next.time - _prev.time);

		// 補間
		a_resullt = DirectX::XMVectorLerp(
			DirectX::XMLoadFloat3(&_prev.vec),
			DirectX::XMLoadFloat3(&_next.vec),
			_f
		);
	}

	return true;
}
