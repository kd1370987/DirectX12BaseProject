// インクルードガード
#ifndef CB_BLOOM_OPTION_HLSLI
#define CB_BLOOM_OPTION_HLSLI

// 川瀬式ブルームの調整値。
// OptionManager の BloomOption を C++ 側が詰めて、抽出パスと合成パスの両方へ送る。
//
//   抽出 : threshold を超えた輝度ぶんだけを取り出す
//          (softKnee でしきい値の境目をなめらかにして、明滅のちらつきを抑える)
//   合成 : メインカラー + ブルーム * intensity
struct BloomOptionData
{
	float threshold;	// 高輝度として抽出し始める輝度
	float softKnee;		// しきい値付近をなめらかにつなぐ幅の割合（0でハードカット）
	float intensity;	// 合成時のブルームの強さ
	int   enable;		// 0 ならブルームを掛けない
};

// ブルーム調整用定数バッファ
cbuffer CBBloomOption : register(b13)
{
	BloomOptionData g_bloom;
}

#endif

// ルートシグネチャ用
#define RS_BLOOM_OPTION_CB "CBV(b13,visibility = SHADER_VISIBILITY_ALL)"
