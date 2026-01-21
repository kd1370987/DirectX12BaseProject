#include "Viewport.h"


void Viewport::Create(UINT a_frameBufferWidth, UINT a_frameBufferHeight)
{
	// ビューポート
	// ウィンドウに対してレンダリング結果をどう表示するかの設定

	// 左上座標
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	// 幅・高さ
	m_viewport.Width = static_cast<float>(a_frameBufferWidth);
	m_viewport.Height = static_cast<float>(a_frameBufferHeight);

	// 深度のマッピング範囲（奥行情報・Zバッファの値）
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
}