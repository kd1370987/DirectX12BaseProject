#include "UI.hlsli"

float2 Rotate(float2 a_pos,float2 a_pivot,float a_angle)
{
	// 中心を軸として回転したいので一度原点に移動
	float2 _pivot = a_pos - a_pivot;
	
	float _sin = sin(a_angle);
	float _cos = cos(a_angle);

	// 2D回転行列。y成分は x*sin + y*cos が正しい。
	// (以前は - y*cos になっており、回転0でもYが反転して上下逆さ・移動が逆方向になっていた)
	_pivot = float2(
		_pivot.x * _cos - _pivot.y * _sin,
		_pivot.x * _sin + _pivot.y * _cos
	);

	return _pivot + a_pivot;
}

[RootSignature(UI_RS)]
VSOutput VSMain(VSInput a_input)
{
	// UIデータを取得
	UIData _uiData = g_uiDataBuffer[a_input.instID];

	// 座標
	float2 _local = a_input.pos.xy * _uiData.size;
	_local = Rotate(_local, _uiData.pivot, _uiData.rotation);
	_local += _uiData.pos;

	// 構造体にしてPSへ
	VSOutput _out;
	_out.pos = float4(_local, _uiData.layer, 1);
	_out.uv = a_input.uv + _uiData.uvOffset;
	_out.color = _uiData.color;
	_out.texIndex = _uiData.texIndex;	// 実際のSRV番号をPSへ渡す
	return _out;
}
