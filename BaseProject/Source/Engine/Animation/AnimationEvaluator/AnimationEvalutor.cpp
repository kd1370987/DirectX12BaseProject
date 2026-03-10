#include "AnimationEvalutor.h"

/// <summary>
/// 二分探索で、指定時間から次の配列要素のkeyIndexを求める
/// </summary>
/// <param name="a_list">キー配列</param>
/// <returns>次の配列要素のインデックス</returns>
template<class T>
int BinarySearchNextAnimKey(const std::vector<T>& a_list, float a_currentTime)
{
	int _low = 0;
	int _high = static_cast<int>(a_list.size());

	while (_low < _high)
	{
		int _mid = (_low + _high) / 2;
		float _midTime = a_list[_mid].time;

		if (_midTime <= a_currentTime)
		{
			_low = _low + 1;
		}
		else
		{
			_high = _mid;
		}
	}
	return _low;
}

void Engine::Animation::Interpolate(Engine::Resource::AnimationNode& a_node, float a_currentTime, DirectX::XMFLOAT4X4& a_rDst)
{
	bool _isChange = false;

	DXSM::Vector3 _rs = {};
	DXSM::Quaternion _rq = {};
	DXSM::Vector3 _rt = {};
	if (InterpolateScale(a_node, a_currentTime, _rs)) _isChange = true;
	if (InterpolateRotations(a_node, a_currentTime, _rq)) _isChange = true;
	if (InterpolateTranslations(a_node, a_currentTime, _rt)) _isChange = true;

	if (_isChange)
	{
		DXSM::Matrix _sMat = DXSM::Matrix::CreateScale(_rs);
		DXSM::Matrix _rMat = DXSM::Matrix::CreateFromQuaternion(_rq);
		DXSM::Matrix _tMat = DXSM::Matrix::CreateTranslation(_rt);
		a_rDst = _sMat * _rMat * _tMat;
	}
}

bool Engine::Animation::InterpolateTranslations(Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt)
{
	if (a_node.translations.size() == 0) return false;

	// キー検索
	UINT _keyIdx = BinarySearchNextAnimKey(a_node.translations, a_currentTime);

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
		float _f = (a_currentTime - _prev.time) / (_next.time - _prev.time);

		// 補間
		a_resullt = DirectX::XMVectorLerp(
			DirectX::XMLoadFloat3(&_prev.vec),
			DirectX::XMLoadFloat3(&_next.vec),
			_f
		);
	}

	return true;
}

bool Engine::Animation::InterpolateRotations(Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Quaternion& a_resullt)
{
	if (a_node.rotations.size() == 0) return false;

	// キー検索
	UINT _keyIdx = BinarySearchNextAnimKey(a_node.rotations, a_currentTime);

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
		float _f = (a_currentTime - _prev.time) / (_next.time - _prev.time);

		// 補間
		a_resullt = DirectX::XMQuaternionSlerp(
			DirectX::XMLoadFloat4(&_prev.quat),
			DirectX::XMLoadFloat4(&_next.quat),
			_f
		);
	}

	return true;
}

bool Engine::Animation::InterpolateScale(Engine::Resource::AnimationNode& a_node, float a_currentTime, DXSM::Vector3& a_resullt)
{
	if (a_node.scales.size() == 0) return false;

	// キー検索
	UINT _keyIdx = BinarySearchNextAnimKey(a_node.scales, a_currentTime);

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
		float _f = (a_currentTime - _prev.time) / (_next.time - _prev.time);

		// 補間
		a_resullt = DirectX::XMVectorLerp(
			DirectX::XMLoadFloat3(&_prev.vec),
			DirectX::XMLoadFloat3(&_next.vec),
			_f
		);
	}

	return true;
}

void Engine::Animation::CalcNodeMatrix(
	int a_nodeIdx, 
	int a_parentNodeIdx,
	const Engine::Resource::Model* a_model,
	DirectX::XMFLOAT4X4* a_pOutLocalMat, 
	DirectX::XMFLOAT4X4* a_pOutWorldMat
)
{
	const auto& _node = a_model->originalNodes[a_nodeIdx];

	if (a_parentNodeIdx >= 0)
	{
		DXSM::Matrix _localMat(a_pOutLocalMat[a_nodeIdx]);
		DXSM::Matrix _parentWorldMat(a_pOutWorldMat[a_parentNodeIdx]);
		DXSM::Matrix _worldMat = _localMat * _parentWorldMat;
		a_pOutWorldMat[a_nodeIdx] = _worldMat;
	}
	else
	{
		a_pOutWorldMat[a_nodeIdx] = a_pOutLocalMat[a_nodeIdx];
	}

	// 子再帰
	for (auto& _childIdx : _node.children)
	{
		CalcNodeMatrix(_childIdx, a_nodeIdx, a_model, a_pOutLocalMat, a_pOutWorldMat);
	}
}
