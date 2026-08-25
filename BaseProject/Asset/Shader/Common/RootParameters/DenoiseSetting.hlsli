// A-Trous 空間デノイズと時間累積の設定。GI と影で同じ形を使う
#ifndef ROOTPARAM_DENOISE_SETTING_HLSLI
#define ROOTPARAM_DENOISE_SETTING_HLSLI

struct SpatialDenoiseSetting
{
	int stepSize;		// パスごとのタップ間隔
	float phiDepth;		// 深度の感度(ビュー深度に対する相対値)。小さいほどエッジを厳密に保護
	float phiNormal;	// 法線の感度(pow の指数)。大きいほど法線のずれに敏感
	float phiColor;		// 輝度の感度(ノイズとディティールの境界)
};

struct TemporalAccumulationSetting
{
	float phiDepth;		// 位置差の許容量(ビュー深度に対する相対値)
	// 法線一致の下限。dot がこれ未満なら履歴を捨てる。
	// 空間側の phiNormal(pow の指数)とは意味が違うので取り違えないこと
	float phiNormal;
	float blendRate;	// 現在フレームの割合
};

#endif
