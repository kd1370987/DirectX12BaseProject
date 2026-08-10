// インクルードガード
#ifndef CB_DOF_OPTION_HLSLI
#define CB_DOF_OPTION_HLSLI

// オプション(DoFOption)からCPU側で詰めて送られてくる被写界深度の調整値
//
//   ピントが合う範囲 : focusDistance ± focusRange * 0.5
//   そこから手前へ nearRange 進むと最大ボケ(CoC = -1)
//   そこから奥へ   farRange  進むと最大ボケ(CoC = +1)
struct DoFOptionData
{
	float focusDistance;	// ピントが合う距離（カメラからの深度）
	float focusRange;		// ピントが合う幅
	float nearRange;		// 手前側が最大ボケになるまでの距離
	float farRange;			// 奥側が最大ボケになるまでの距離

	float maxBlurRadius;	// 最大ボケ半径（ピクセル）
	int enable;				// 0 ならボカさずそのまま通す
	float2 pad;
};

// 被写界深度調整用定数バッファ
cbuffer CBDoFOption : register(b12)
{
	DoFOptionData g_dof;
}

#endif

// ルートシグネチャ用
#define RS_DOF_OPTION_CB "CBV(b12,visibility = SHADER_VISIBILITY_ALL)"
