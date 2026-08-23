#include "UI.hlsli"

[RootSignature(UI_RS)]
VSOutput VSMain(VSInput a_input)
{
	// UIデータを取得
	UIData _uiData = g_uiDataBuffer[a_input.instID];

	// 回転・アスペクト補正・ピボットはCPU(SubmitUI)側でピクセル空間で計算し、
	// クアッド頂点(-1..1)を線形変換する基底(axisX/axisY)とNDC中心(pos)として渡している。
	// ここでは基底に沿ってクアッド頂点を配置するだけ。
	float2 _q = a_input.pos.xy;	// -1..1
	float2 _ndc = _uiData.pos + _uiData.axisX * _q.x + _uiData.axisY * _q.y;

	// 構造体にしてPSへ
	//
	// Zは常に0。layer は「どの順で描くか」を決めるためのものでCPU側が並べ替えに使う。
	// ここへ入れてしまうと、深度を使っていないので重なりには効かないのに、
	// 0〜1 の外を指定したときだけクリップされて絵が消える
	VSOutput _out;
	_out.pos = float4(_ndc, 0.0f, 1);
	// 倍率 → オフセットの順。1枚に並べた絵から1コマだけ切り出すときは
	// 倍率でコマの大きさ、オフセットで何コマ目かを指定する
	_out.uv = a_input.uv * _uiData.uvScale + _uiData.uvOffset;
	_out.color = _uiData.color;
	_out.texIndex = _uiData.texIndex;	// 実際のSRV番号をPSへ渡す
	return _out;
}
