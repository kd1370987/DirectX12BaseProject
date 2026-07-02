#include "DebugLineRoot.hlsli"

#include "../../Common/Shape/Box.hlsli"
#include "../../Common/Shape/Capsule.hlsli"
#include "../../Common/Shape/Sphere.hlsli"

// ルートシグネチャ定義
[RootSignature(DEBUGLINE_ROOT_SIG)]

VSOutput VSMain(VSInput a_input)
{
	int _index = a_input.instID;
	DebugLine _instance = g_debuglineBuffer[_index];
	
	float3 _localPos = float3(0, 0, 0);
	bool _isValid = true; // 描画してよい頂点かどうか

	if (_instance.shapeType == 0)
	{
		// レイ : 2頂点
		if (a_input.vertexID == 0)
		{
			_localPos = float3(0, 0, 0);
		}
		else if (a_input.vertexID == 1)
		{
			_localPos = float3(0, 0, 1);
		}
		else
		{
			_isValid = false;
		}
	}
	else if (_instance.shapeType == 1)
	{
		// ボックス : 24頂点
		if (a_input.vertexID < 24)
		{
			_localPos = BOX_VERTICES[a_input.vertexID];
		}
		else
		{
			_isValid = false;
		}
	}
	else if (_instance.shapeType == 2)
	{
		// カプセル : 136頂点
		if (a_input.vertexID < 136)
		{
			_localPos = GetCapsulePoint(a_input.vertexID);
		}
		else
		{
			_isValid = false;
		}
	}
	else if (_instance.shapeType == 3)
	{
		// スフィア : 96頂点
		if (a_input.vertexID < 96)
		{
			_localPos = GetSpherePoint(a_input.vertexID);
		}
		else
		{
			_isValid = false;
		}
	}
	else
	{
		_isValid = false;
	}

	VSOutput _outPut = (VSOutput) 0;

	if (!_isValid)
	{
		// 不要な頂点はW=0にしてラスタライザにクリップさせる
		_outPut.svPos = float4(0, 0, 0, 0);
		return _outPut;
	}

	// 投影変換
	_outPut.svPos = Transform_LocalToProj(_localPos, _instance.worldMat);
	_outPut.color = _instance.color;
	
	return _outPut;
}
