// UI1枚ぶんの描画データ。
//
// 回転・アスペクト補正・ピボットはCPU側がピクセル空間で解決済みで、
// シェーダーは pos + axisX*q.x + axisY*q.y の線形変換をするだけでよい。
//
// ※ CPU 側 Engine::Graphics::UIData と並びを合わせること。
//    float2 が16バイト境界をまたぐ位置に来ると次の境界へ押し出されるので、
//    各行がちょうど float4 に収まる順序を崩さないこと
//    (row0: pos+axisX / row1: axisY+uvOffset / row2: color / row3: layer+texIndex+uvScale)
#ifndef ROOTPARAM_UI_DATA_HLSLI
#define ROOTPARAM_UI_DATA_HLSLI

struct UIData
{
	float2 pos;			// クアッド中心(NDC)
	float2 axisX;		// クアッドx方向の基底(NDC)

	float2 axisY;		// クアッドy方向の基底(NDC)
	float2 uvOffset;

	float4 color;

	float layer;		// 重なり順。CPU側の並べ替え用で、VSでは読まない
	uint texIndex;		// SRVインデックス
	float2 uvScale;		// uv * uvScale + uvOffset。既定は(1,1)
};

#endif
